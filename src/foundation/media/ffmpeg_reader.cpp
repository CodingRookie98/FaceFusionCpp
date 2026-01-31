/**
 * ******************************************************************************
 * @file           : ffmpeg_reader.cpp
 * @brief          : VideoReader implementation
 * ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <iostream>
#include <format>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

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
import foundation.infrastructure.concurrent_queue;

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
    std::atomic<int64_t> current_pts = 0;
    double time_base = 0.0;
    int frame_count = 0;
    double fps = 0.0;
    int width = 0;
    int height = 0;
    int64_t duration_ms = 0;

    // Async support
    ConcurrentQueue<cv::Mat> frame_queue{32}; // Max 32 frames buffer
    std::thread decoding_thread;
    std::atomic<bool> is_decoding = false;
    std::atomic<bool> seek_requested = false;
    std::atomic<int64_t> seek_target_ts = 0;
    std::mutex seek_mutex;
    std::condition_variable seek_cv;

    // We still need to handle the case where read_frame returns empty to signal EOF
    // So queue might return optional, nullopt means EOF or closed.
    // In our ConcurrentQueue, pop returns optional. nullopt is only when shutdown.
    // So we need a special marker or just shutdown the queue on EOF.
    // Let's use shutdown on EOF.

    ~Impl() { cleanup(); }

    void cleanup() {
        stop_decoding();

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

    void start_decoding() {
        if (is_decoding) return;
        is_decoding = true;
        frame_queue.reset();
        decoding_thread = std::thread(&Impl::decoding_loop, this);
    }

    void stop_decoding() {
        if (!is_decoding) return;
        is_decoding = false;
        frame_queue.shutdown(); // Wake up any waiting consumers/producers
        if (decoding_thread.joinable()) { decoding_thread.join(); }
    }

    void decoding_loop() {
        while (is_decoding) {
            // Check for seek request
            if (seek_requested) {
                std::lock_guard<std::mutex> lock(seek_mutex);

                // Flush queue
                frame_queue.clear();

                // Flush codec
                if (codec_ctx) { avcodec_flush_buffers(codec_ctx); }

                // Perform seek
                // Use AVSEEK_FLAG_BACKWARD to find nearest keyframe before target
                if (av_seek_frame(format_ctx, video_stream_index, seek_target_ts,
                                  AVSEEK_FLAG_BACKWARD)
                    >= 0) {
                    // We might need to decode until we reach the exact frame if precision is
                    // required, but for now let's just seek and continue decoding. The consumer
                    // expects the stream to continue from new position. Note: Precise seeking
                    // usually requires decoding from keyframe to target. Let's implement simple
                    // seeking first: just seek to keyframe. If precise seek is needed, we should do
                    // it here but do not push frames until target is reached.
                } else {
                    Logger::get_instance()->error("VideoReader: Seek failed inside decoding loop");
                }

                seek_requested = false;
                seek_cv.notify_one(); // Notify main thread that seek is done
            }

            int ret = av_read_frame(format_ctx, packet);
            if (ret < 0) {
                // EOF or error
                if (ret == AVERROR_EOF) {
                    // Signal EOF by shutting down queue?
                    // Or push an empty Mat? Let's shutdown queue for now to signal end of stream.
                    // But shutdown implies error or close.
                    // Let's just break loop, but keep is_decoding true?
                    // No, if EOF, we should stop.
                    // But wait, if we stop, pop() will return nullopt immediately if empty.
                    // That works for EOF signal.
                    break;
                }
                continue;
            }

            if (packet->stream_index == video_stream_index) {
                ret = avcodec_send_packet(codec_ctx, packet);
                av_packet_unref(packet);

                if (ret < 0) continue;

                while (ret >= 0) {
                    ret = avcodec_receive_frame(codec_ctx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
                    if (ret < 0) break;

                    // Update current pts (atomic)
                    // Note: This might be updated "ahead" of what is being read by consumer.
                    // If consumer relies on this for the frame being read, we might need to attach
                    // timestamp to the frame. But cv::Mat doesn't hold timestamp easily without
                    // extra wrapper. For now, let's assume get_current_timestamp_ms() is
                    // approximate or used for seeking status.
                    current_pts = frame->best_effort_timestamp;

                    // Convert to BGR
                    // Optimized: Reuse frame_bgr buffer
                    sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, frame_bgr->data,
                              frame_bgr->linesize);

                    // Create cv::Mat (copy data)
                    // TODO: Optimization - if possible, map AVFrame data to Mat if continuous and
                    // not padded. FFmpeg linesize usually includes padding (alignment). Mat needs
                    // to be created with step. But standard Mat constructor with data assumes no
                    // padding or user specified step.

                    // Optimized copy using linesize aware copy, still needed because Mat expects
                    // contiguous or specific step We can create Mat with step if we want to avoid
                    // copy, but we need to ensure the data persists. Since frame_bgr is reused, we
                    // MUST copy.

                    cv::Mat mat(height, width, CV_8UC3);
                    // Use libavutil to copy if possible or manual copy
                    // Manual copy is fine but let's check alignment
                    uint8_t* dst = mat.data;
                    int dst_stride = static_cast<int>(mat.step[0]);
                    const uint8_t* src = frame_bgr->data[0];
                    int src_stride = frame_bgr->linesize[0];

                    if (src_stride == dst_stride) {
                        // Single block copy if strides match and no gap
                        // But usually stride > width*3.
                        // If strides match, we can copy height * stride?
                        // Be careful about memory safety.
                        // Safe approach: copy row by row.
                        for (int y = 0; y < height; y++) {
                            memcpy(dst + y * dst_stride, src + y * src_stride, width * 3);
                        }
                    } else {
                        for (int y = 0; y < height; y++) {
                            memcpy(dst + y * dst_stride, src + y * src_stride, width * 3);
                        }
                    }

                    frame_queue.push(std::move(mat));
                    av_frame_unref(frame);
                }
            } else {
                av_packet_unref(packet);
            }
        }

        // FLUSH DECODER
        if (codec_ctx) {
            avcodec_send_packet(codec_ctx, nullptr);
            while (is_decoding) {
                int ret = avcodec_receive_frame(codec_ctx, frame);
                if (ret < 0) break;

                current_pts = frame->best_effort_timestamp;
                sws_scale(sws_ctx, frame->data, frame->linesize, 0, height, frame_bgr->data,
                          frame_bgr->linesize);

                cv::Mat mat(height, width, CV_8UC3);
                for (int y = 0; y < height; y++) {
                    memcpy(mat.data + y * mat.step[0],
                           frame_bgr->data[0] + y * frame_bgr->linesize[0], width * 3);
                }
                frame_queue.push(std::move(mat));
                av_frame_unref(frame);
            }
        }

        // When loop ends (EOF or stop), we shutdown queue so consumers know no more frames coming
        frame_queue.shutdown();
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

        // Enable multithreading for decoding if CPU allows
        // ffmpeg usually handles this internally if configured
        codec_ctx->thread_count = 0; // 0 means auto

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

        // Start async decoding
        start_decoding();

        return true;
    }

    cv::Mat read_frame() {
        if (!is_open) { return {}; }

        // Pop from queue
        auto mat_opt = frame_queue.pop();
        if (mat_opt) { return *mat_opt; }

        // If nullopt, queue is shutdown (EOF or error)
        return {};
    }

    bool seek(int64_t frame_index) {
        if (!is_open || frame_index < 0) { return false; }

        // Calculate target timestamp in stream time base
        // frame_index / fps = seconds
        // seconds / time_base = stream units
        int64_t target_ts = static_cast<int64_t>((double)frame_index / fps / time_base);

        stop_decoding();

        avcodec_flush_buffers(codec_ctx);

        if (av_seek_frame(format_ctx, video_stream_index, target_ts, AVSEEK_FLAG_BACKWARD) < 0) {
            Logger::get_instance()->error("VideoReader: Seek failed");
            start_decoding();
            return false;
        }

        // Synchronous precise seek
        bool found = false;
        int max_frames_to_skip = 1000; // Safety break
        int frames_skipped = 0;

        while (!found && frames_skipped < max_frames_to_skip) {
            int ret = av_read_frame(format_ctx, packet);
            if (ret < 0) break;

            if (packet->stream_index == video_stream_index) {
                if (avcodec_send_packet(codec_ctx, packet) == 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) == 0) {
                        int64_t current_ts = frame->best_effort_timestamp;
                        if (current_ts == AV_NOPTS_VALUE) current_ts = frame->pts;

                        double current_sec = current_ts * time_base;
                        int64_t current_frame = static_cast<int64_t>(current_sec * fps + 0.5);

                        if (current_frame >= frame_index) {
                            sws_scale(sws_ctx, frame->data, frame->linesize, 0, height,
                                      frame_bgr->data, frame_bgr->linesize);
                            cv::Mat mat(height, width, CV_8UC3);
                            for (int y = 0; y < height; y++) {
                                memcpy(mat.data + y * mat.step[0],
                                       frame_bgr->data[0] + y * frame_bgr->linesize[0], width * 3);
                            }

                            frame_queue.clear();
                            frame_queue.push(std::move(mat));
                            current_pts = current_ts;
                            found = true;
                        } else {
                            frames_skipped++;
                        }
                        av_frame_unref(frame);
                        if (found) break;
                    }
                }
            }
            av_packet_unref(packet);
        }

        start_decoding();
        return found;
    }

    bool seek_by_time(double timestamp_ms) {
        if (fps <= 0.000001) return false;
        int64_t frame_idx = static_cast<int64_t>(timestamp_ms / 1000.0 * fps);
        return seek(frame_idx);
    }

    double get_current_timestamp_ms() const {
        if (!is_open) { return 0.0; }
        return current_pts * time_base * 1000.0;
    }

    int64_t get_current_frame() const {
        if (fps <= 0) return 0;
        return static_cast<int64_t>(get_current_timestamp_ms() * fps / 1000.0);
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
bool VideoReader::seek_by_time(double timestamp_ms) {
    return impl_->seek_by_time(timestamp_ms);
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

int64_t VideoReader::get_current_frame() const {
    return impl_->get_current_frame();
}

} // namespace foundation::media::ffmpeg
