/**
 ******************************************************************************
 * @file           : ffmpeg_reader.cpp
 * @brief          : VideoReader implementation
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <iostream>
#include <format>
#include <filesystem>

#include <opencv2/opencv.hpp>

module foundation.media.ffmpeg;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

import foundation.infrastructure.logger;

namespace foundation::media::ffmpeg {

using namespace foundation::infrastructure;
using namespace foundation::infrastructure::logger;

// ============================================================================
// VideoReader Implementation
// ============================================================================

struct VideoReader::Impl {
    std::string video_path;
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVFrame* frame_bgr = nullptr;
    AVPacket* packet = nullptr;
    int video_stream_index = -1;
    bool is_open = false;
    int64_t current_pts = 0;
    double time_base = 0.0;
    int frame_count = 0;
    double fps = 0.0;
    int width = 0;
    int height = 0;
    int64_t duration_ms = 0;

    ~Impl() { cleanup(); }

    void cleanup() {
        if (sws_ctx) {
            sws_freeContext(sws_ctx);
            sws_ctx = nullptr;
        }
        if (frame_bgr) {
            av_frame_free(&frame_bgr);
            frame_bgr = nullptr;
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
            avformat_close_input(&format_ctx);
            format_ctx = nullptr;
        }
        is_open = false;
    }

    bool open() {
        cleanup();

        if (avformat_open_input(&format_ctx, video_path.c_str(), nullptr, nullptr) < 0) {
            Logger::get_instance()->error(
                std::format("VideoReader: Failed to open input file: {}", video_path));
            return false;
        }

        if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
            Logger::get_instance()->error("VideoReader: Failed to find stream info");
            cleanup();
            return false;
        }

        // Find video stream
        for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_index = static_cast<int>(i);
                break;
            }
        }

        if (video_stream_index < 0) {
            Logger::get_instance()->error("VideoReader: No video stream found");
            cleanup();
            return false;
        }

        AVStream* video_stream = format_ctx->streams[video_stream_index];
        AVCodecParameters* codecpar = video_stream->codecpar;

        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            Logger::get_instance()->error("VideoReader: Decoder not found");
            cleanup();
            return false;
        }

        codec_ctx = avcodec_alloc_context3(codec);
        if (!codec_ctx) {
            Logger::get_instance()->error("VideoReader: Failed to allocate codec context");
            cleanup();
            return false;
        }

        if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
            Logger::get_instance()->error("VideoReader: Failed to copy codec params");
            cleanup();
            return false;
        }

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            Logger::get_instance()->error("VideoReader: Failed to open codec");
            cleanup();
            return false;
        }

        frame = av_frame_alloc();
        frame_bgr = av_frame_alloc();
        packet = av_packet_alloc();

        if (!frame || !frame_bgr || !packet) {
            Logger::get_instance()->error("VideoReader: Failed to allocate frame/packet");
            cleanup();
            return false;
        }

        // Setup BGR frame
        width = codec_ctx->width;
        height = codec_ctx->height;
        frame_bgr->format = AV_PIX_FMT_BGR24;
        frame_bgr->width = width;
        frame_bgr->height = height;
        av_frame_get_buffer(frame_bgr, 32);

        sws_ctx = sws_getContext(width, height, codec_ctx->pix_fmt, width, height, AV_PIX_FMT_BGR24,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!sws_ctx) {
            Logger::get_instance()->error("VideoReader: Failed to create scaler context");
            cleanup();
            return false;
        }

        // Calculate metadata
        time_base = av_q2d(video_stream->time_base);
        fps = av_q2d(video_stream->avg_frame_rate);

        // Sanity check for FPS (e.g. 0.1 to 200 fps)
        if (fps < 0.1 || fps > 200.0) {
            double r_fps = av_q2d(video_stream->r_frame_rate);
            if (r_fps >= 0.1 && r_fps <= 200.0) {
                fps = r_fps;
            } else {
                fps = 30.0; // Default fallback
            }
        }

        if (video_stream->nb_frames > 0) {
            frame_count = static_cast<int>(video_stream->nb_frames);
        } else if (video_stream->duration > 0) {
            frame_count = static_cast<int>(video_stream->duration * fps * time_base);
        } else if (format_ctx->duration > 0) {
            frame_count = static_cast<int>((format_ctx->duration / AV_TIME_BASE) * fps);
        }

        if (format_ctx->duration > 0) {
            duration_ms = format_ctx->duration / 1000; // microseconds to milliseconds
        } else if (video_stream->duration > 0) {
            duration_ms = static_cast<int64_t>(video_stream->duration * time_base * 1000);
        }

        is_open = true;
        return true;
    }

    cv::Mat read_frame() {
        if (!is_open) { return {}; }

        while (av_read_frame(format_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_index) {
                int ret = avcodec_send_packet(codec_ctx, packet);
                av_packet_unref(packet);

                if (ret < 0) { continue; }

                ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { continue; }
                if (ret < 0) { continue; }

                // Update current pts
                current_pts = frame->best_effort_timestamp;

                // Convert to BGR
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, frame_bgr->data,
                          frame_bgr->linesize);

                // Create cv::Mat (copy data)
                cv::Mat mat(height, width, CV_8UC3);
                for (int y = 0; y < height; y++) {
                    memcpy(mat.ptr(y), frame_bgr->data[0] + y * frame_bgr->linesize[0], width * 3);
                }

                av_frame_unref(frame);
                return mat;
            }
            av_packet_unref(packet);
        }

        return {}; // EOF
    }

    bool seek(int64_t frame_index) {
        if (!is_open || frame_index < 0) { return false; }

        AVStream* video_stream = format_ctx->streams[video_stream_index];
        int64_t timestamp = static_cast<int64_t>(frame_index / fps / time_base);

        if (av_seek_frame(format_ctx, video_stream_index, timestamp, AVSEEK_FLAG_BACKWARD) < 0) {
            Logger::get_instance()->error("VideoReader: Seek failed");
            return false;
        }

        avcodec_flush_buffers(codec_ctx);
        return true;
    }

    double get_current_timestamp_ms() const {
        if (!is_open) { return 0.0; }
        return current_pts * time_base * 1000.0;
    }
};

VideoReader::VideoReader(const std::string& videoPath) : impl_(std::make_unique<Impl>()) {
    impl_->video_path = videoPath;
}

VideoReader::~VideoReader() = default;

VideoReader::VideoReader(VideoReader&&) noexcept = default;
VideoReader& VideoReader::operator=(VideoReader&&) noexcept = default;

bool VideoReader::open() {
    return impl_->open();
}
void VideoReader::close() {
    impl_->cleanup();
}
bool VideoReader::is_opened() const {
    return impl_->is_open;
}
cv::Mat VideoReader::read_frame() {
    return impl_->read_frame();
}
bool VideoReader::seek(int64_t frame_index) {
    return impl_->seek(frame_index);
}
int VideoReader::get_frame_count() const {
    return impl_->frame_count;
}
double VideoReader::get_fps() const {
    return impl_->fps;
}
int VideoReader::get_width() const {
    return impl_->width;
}
int VideoReader::get_height() const {
    return impl_->height;
}
int64_t VideoReader::get_duration_ms() const {
    return impl_->duration_ms;
}
double VideoReader::get_current_timestamp_ms() const {
    return impl_->get_current_timestamp_ms();
}

} // namespace foundation::media::ffmpeg
