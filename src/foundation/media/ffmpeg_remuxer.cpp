/**
 * @file ffmpeg_remuxer.cpp
 * @brief Remuxer implementation
 */
module;

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
}

module foundation.media.ffmpeg;
import :remuxer;

namespace foundation::media::ffmpeg {

// Helper RAII wrapper for AVFormatContext
struct FormatContextPtr {
    AVFormatContext* ptr = nullptr;
    bool is_output = false;
    ~FormatContextPtr() {
        if (ptr) {
            if (is_output) {
                if (ptr->pb) avio_closep(&ptr->pb);
                avformat_free_context(ptr);
            } else {
                avformat_close_input(&ptr);
            }
        }
    }
};

bool Remuxer::merge_av(const std::string& video_path, const std::string& audio_path,
                       const std::string& output_path) {
    FormatContextPtr in_video_ctx, in_audio_ctx, out_ctx;

    // 1. Open Input Video
    if (avformat_open_input(&in_video_ctx.ptr, video_path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Remuxer: Failed to open video input: " << video_path << std::endl;
        return false;
    }
    if (avformat_find_stream_info(in_video_ctx.ptr, nullptr) < 0) return false;

    // 2. Open Input Audio
    if (avformat_open_input(&in_audio_ctx.ptr, audio_path.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Remuxer: Failed to open audio input: " << audio_path << std::endl;
        return false;
    }
    if (avformat_find_stream_info(in_audio_ctx.ptr, nullptr) < 0) return false;

    // 3. Create Output Context
    avformat_alloc_output_context2(&out_ctx.ptr, nullptr, nullptr, output_path.c_str());
    if (!out_ctx.ptr) {
        std::cerr << "Remuxer: Failed to create output context" << std::endl;
        return false;
    }
    out_ctx.is_output = true;

    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    int out_video_stream_idx = -1;
    int out_audio_stream_idx = -1;

    // 4. Find and Copy Video Stream
    for (unsigned int i = 0; i < in_video_ctx.ptr->nb_streams; i++) {
        if (in_video_ctx.ptr->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            AVStream* in_stream = in_video_ctx.ptr->streams[i];
            AVStream* out_stream = avformat_new_stream(out_ctx.ptr, nullptr);
            if (!out_stream) return false;
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
            out_video_stream_idx = out_stream->index;
            break;
        }
    }

    // 5. Find and Copy Audio Stream
    for (unsigned int i = 0; i < in_audio_ctx.ptr->nb_streams; i++) {
        if (in_audio_ctx.ptr->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            AVStream* in_stream = in_audio_ctx.ptr->streams[i];
            AVStream* out_stream = avformat_new_stream(out_ctx.ptr, nullptr);
            if (!out_stream) return false;
            avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            out_stream->codecpar->codec_tag = 0;
            out_audio_stream_idx = out_stream->index;
            break;
        }
    }

    if (video_stream_idx == -1) {
        std::cerr << "Remuxer: No video stream found in input" << std::endl;
        return false;
    }

    // 6. Open Output File
    if (!(out_ctx.ptr->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_ctx.ptr->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Remuxer: Failed to open output file" << std::endl;
            return false;
        }
    }

    // 7. Write Header
    if (avformat_write_header(out_ctx.ptr, nullptr) < 0) return false;

    // 8. Copy Packets
    std::vector<int> stream_mapping(
        std::max(in_video_ctx.ptr->nb_streams, in_audio_ctx.ptr->nb_streams), -1);

    AVPacket* pkt = av_packet_alloc();
    if (!pkt) return false;

    // We need to read from both files. This is simplified: we read frames from both strategies
    // separately? Actually, usually we loop until both are EOF. But here we have two INPUT files.
    // Simpler approach: Copy all video packets, then all audio packets?
    // NO, MP4 requires interleaving.

    // Better strategy: Read one packet from A, one from B?
    // Or simpler: Just copy video stream, then restart and copy audio stream?
    // ffmpeg muxer handles interleaving somewhat if we use interleaved_write_frame.
    // However, monotonic DTS/PTS is preferred.
    // Let's implement a loop that polls both inputs.

    int64_t video_pts = 0;
    int64_t audio_pts = 0;
    bool video_eof = false;
    bool audio_eof = false;

    // Because libraries like to read sequentially, interleaved reading from two files is tricky
    // without lookahead. However, since we are just remuxing, we can read chunks. Or we simply
    // loop: read video, write; read audio, write. The safest "simple" way is: read packet from
    // video context. write. check timestamp. read packet from audio context. write. This assumes
    // somewhat synchronized inputs.

    // Let's prioritize video.

    while (!video_eof || (audio_stream_idx != -1 && !audio_eof)) {
        // Simple sequential round-robin
        // Note: Real muxing requires checking DTS to decide which stream to pick next.
        // For this task, strict time alignment isn't critical if we just rely on
        // interleaved_write_frame to buffer.

        if (!video_eof) {
            int ret = av_read_frame(in_video_ctx.ptr, pkt);
            if (ret < 0) {
                video_eof = true;
            } else {
                if (pkt->stream_index == video_stream_idx) {
                    AVStream* in_stream = in_video_ctx.ptr->streams[video_stream_idx];
                    AVStream* out_stream = out_ctx.ptr->streams[out_video_stream_idx];

                    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
                    pkt->pos = -1;
                    pkt->stream_index = out_video_stream_idx;

                    av_interleaved_write_frame(out_ctx.ptr, pkt);
                }
                av_packet_unref(pkt);
            }
        }

        if (audio_stream_idx != -1 && !audio_eof) {
            int ret = av_read_frame(in_audio_ctx.ptr, pkt);
            if (ret < 0) {
                audio_eof = true;
            } else {
                if (pkt->stream_index == audio_stream_idx) {
                    AVStream* in_stream = in_audio_ctx.ptr->streams[audio_stream_idx];
                    AVStream* out_stream = out_ctx.ptr->streams[out_audio_stream_idx];

                    av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
                    pkt->pos = -1;
                    pkt->stream_index = out_audio_stream_idx;

                    // Duration Check: Stop writing audio if it exceeds video duration
                    // significantly? For now, write everything.

                    av_interleaved_write_frame(out_ctx.ptr, pkt);
                }
                av_packet_unref(pkt);
            }
        }
    }

    av_write_trailer(out_ctx.ptr);
    av_packet_free(&pkt);

    return true;
}

} // namespace foundation::media::ffmpeg
