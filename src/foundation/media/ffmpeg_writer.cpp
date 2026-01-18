/**
 ******************************************************************************
 * @file           : ffmpeg_writer.cpp
 * @brief          : VideoWriter implementation
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <iostream>
#include <format>
#include <filesystem>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

module foundation.media.ffmpeg;
import foundation.infrastructure.logger;

namespace foundation::media::ffmpeg {

using namespace foundation::infrastructure;
using namespace foundation::infrastructure::logger;

// ============================================================================
// VideoWriter Implementation
// ============================================================================

struct VideoWriter::Impl {
    std::string output_path;
    std::string audio_source_path;
    VideoPrams params;
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    AVStream* video_stream = nullptr;
    bool is_open = false;
    int written_frame_count = 0;
    int64_t next_pts = 0;

    explicit Impl(const VideoPrams& p) : params(p) {}

    ~Impl() { cleanup(); }

    void cleanup() {
        if (is_open && format_ctx && format_ctx->pb) {
            // Flush encoder
            if (codec_ctx) {
                avcodec_send_frame(codec_ctx, nullptr);
                while (avcodec_receive_packet(codec_ctx, packet) == 0) {
                    av_packet_rescale_ts(packet, codec_ctx->time_base, video_stream->time_base);
                    packet->stream_index = video_stream->index;
                    av_interleaved_write_frame(format_ctx, packet);
                    av_packet_unref(packet);
                }
            }
            av_write_trailer(format_ctx);
        }

        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = nullptr;
        }
        if (frame) {
            av_frame_free(&frame);
            frame = nullptr;
        }
        if (packet) {
            av_packet_free(&packet);
            packet = nullptr;
        }
        if (codec_ctx) {
            avcodec_free_context(&codec_ctx);
            codec_ctx = nullptr;
        }
        if (format_ctx) {
            if (format_ctx->pb) { avio_closep(&format_ctx->pb); }
            avformat_free_context(format_ctx);
            format_ctx = nullptr;
        }
        video_stream = nullptr;
        is_open = false;
    }

    bool open() {
        cleanup();

        // Allocate output format context
        if (avformat_alloc_output_context2(&format_ctx, nullptr, nullptr, output_path.c_str())
            < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to allocate output context");
            return false;
        }

        // Find encoder
        std::string codec_name = params.videoCodec.empty() ? "libx264" : params.videoCodec;
        const AVCodec* codec = avcodec_find_encoder_by_name(codec_name.c_str());
        if (!codec) {
            // Fallback to libx264
            codec = avcodec_find_encoder_by_name("libx264");
            if (!codec) {
                // Last resort fallback to mpeg4
                codec = avcodec_find_encoder_by_name("mpeg4");
                if (!codec) {
                    Logger::get_instance()->error("VideoWriter: Encoder not found");
                    cleanup();
                    return false;
                }
            }
        }

        // Create video stream
        video_stream = avformat_new_stream(format_ctx, codec);
        if (!video_stream) {
            Logger::get_instance()->error("VideoWriter: Failed to create stream");
            cleanup();
            return false;
        }

        // Create codec context
        codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            Logger::get_instance()->error("VideoWriter: Failed to allocate codec context");
            cleanup();
            return false;
        }

        // Configure encoder
        codec_ctx->width = static_cast<int>(params.width);
        codec_ctx->height = static_cast<int>(params.height);
        codec_ctx->framerate = AVRational{static_cast<int>(params.frameRate * 1000), 1000};
        codec_ctx->time_base = av_inv_q(codec_ctx->framerate);
        codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        codec_ctx->gop_size = 12;
        codec_ctx->max_b_frames = 2;

        // Set quality (CRF for x264)
        if (codec_name == "libx264" || codec_name == "libx265") {
            int crf = static_cast<int>(51 - params.quality * 0.51);
            av_opt_set(codec_ctx->priv_data, "crf", std::to_string(crf).c_str(), 0);
            if (!params.preset.empty()) {
                av_opt_set(codec_ctx->priv_data, "preset", params.preset.c_str(), 0);
            }
        }

        if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        // Open codec
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to open encoder");
            cleanup();
            return false;
        }

        // Copy codec params to stream
        if (avcodec_parameters_from_context(video_stream->codecpar, codec_ctx) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to copy codec params to stream");
            cleanup();
            return false;
        }

        video_stream->time_base = codec_ctx->time_base;

        // Open output file
        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&format_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
                Logger::get_instance()->error(
                    std::format("VideoWriter: Failed to open output file: {}", output_path));
                cleanup();
                return false;
            }
        }

        // Write header
        if (avformat_write_header(format_ctx, nullptr) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to write header");
            cleanup();
            return false;
        }

        // Allocate frame
        frame = av_frame_alloc();
        packet = av_packet_alloc();
        if (!frame || !packet) {
            Logger::get_instance()->error("VideoWriter: Failed to allocate frame/packet");
            cleanup();
            return false;
        }

        frame->format = codec_ctx->pix_fmt;
        frame->width = codec_ctx->width;
        frame->height = codec_ctx->height;
        av_frame_get_buffer(frame, 32);

        // Create scaler (BGR24 -> YUV420P)
        sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGR24,
                                 codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!sws_ctx) {
            Logger::get_instance()->error("VideoWriter: Failed to create scaler context");
            cleanup();
            return false;
        }

        is_open = true;
        written_frame_count = 0;
        next_pts = 0;
        return true;
    }

    bool write_frame(const cv::Mat& mat) {
        if (!is_open) { return false; }
        if (mat.empty() || mat.cols != codec_ctx->width || mat.rows != codec_ctx->height) {
            Logger::get_instance()->error("VideoWriter: Invalid frame dimensions");
            return false;
        }

        // Make frame writable
        if (av_frame_make_writable(frame) < 0) {
            Logger::get_instance()->error("VideoWriter: Frame not writable");
            return false;
        }

        // Convert BGR to YUV420P
        const uint8_t* src_data[1] = {mat.data};
        int src_linesize[1] = {static_cast<int>(mat.step[0])};
        sws_scale(sws_ctx, src_data, src_linesize, 0, codec_ctx->height, frame->data,
                  frame->linesize);

        frame->pts = next_pts++;

        // Encode frame
        int ret = avcodec_send_frame(codec_ctx, frame);
        if (ret < 0) {
            Logger::get_instance()->error("VideoWriter: Error sending frame to encoder");
            return false;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }
            if (ret < 0) {
                Logger::get_instance()->error("VideoWriter: Error receiving packet from encoder");
                return false;
            }

            av_packet_rescale_ts(packet, codec_ctx->time_base, video_stream->time_base);
            packet->stream_index = video_stream->index;

            ret = av_interleaved_write_frame(format_ctx, packet);
            av_packet_unref(packet);
            if (ret < 0) {
                Logger::get_instance()->error("VideoWriter: Error writing packet");
                return false;
            }
        }

        written_frame_count++;
        return true;
    }
};

VideoWriter::VideoWriter(const std::string& outputPath, const VideoPrams& params) :
    impl_(std::make_unique<Impl>(params)) {
    impl_->output_path = outputPath;
}

VideoWriter::~VideoWriter() = default;

VideoWriter::VideoWriter(VideoWriter&&) noexcept = default;
VideoWriter& VideoWriter::operator=(VideoWriter&&) noexcept = default;

bool VideoWriter::open() {
    return impl_->open();
}
void VideoWriter::close() {
    impl_->cleanup();
}
bool VideoWriter::is_opened() const {
    return impl_->is_open;
}
bool VideoWriter::write_frame(const cv::Mat& frame) {
    return impl_->write_frame(frame);
}
int VideoWriter::get_written_frame_count() const {
    return impl_->written_frame_count;
}

void VideoWriter::set_audio_source(const std::string& sourceVideoPath) {
    impl_->audio_source_path = sourceVideoPath;
}

} // namespace foundation::media::ffmpeg
