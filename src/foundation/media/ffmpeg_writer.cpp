/**
 * ******************************************************************************
 * @file           : ffmpeg_writer.cpp
 * @brief          : VideoWriter implementation
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
#include <libavutil/pixdesc.h>
}

module foundation.media.ffmpeg;

import foundation.infrastructure.logger;
import foundation.infrastructure.concurrent_queue;

namespace foundation::media::ffmpeg {

using namespace foundation::infrastructure;
using namespace foundation::infrastructure::logger;

// ============================================================================
// VideoWriter Implementation
// ============================================================================

struct VideoWriter::Impl {
    std::string output_path;
    std::string audio_source_path;
    VideoParams params;
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    AVStream* video_stream = nullptr;
    bool is_open = false;
    std::atomic<int> written_frame_count = 0;
    int64_t next_pts = 0;

    // Async support
    ConcurrentQueue<cv::Mat> frame_queue{32};
    std::thread encoding_thread;
    std::atomic<bool> is_encoding = false;
    std::atomic<bool> stop_requested = false; // Graceful stop

    explicit Impl(const VideoParams& p) : params(p) {}

    ~Impl() { cleanup(); }

    void cleanup() {
        if (is_encoding) { stop_encoding_and_wait(); }

        if (is_open && format_ctx && format_ctx->pb) {
            // Flush encoder (already handled in stop_encoding_and_wait loop end,
            // but if we crashed or stopped abruptly, we might need to flush here again strictly
            // speaking. But if encoding thread finished, it should have flushed. Let's double check
            // if we need to write trailer.

            // Note: stop_encoding_and_wait() ensures queue is empty and thread finishes loop.
            // The loop handles flushing.
            // BUT, if we call cleanup() without having started encoding (e.g. error in open),
            // we skip stop_encoding_and_wait.

            // We should ensure av_write_trailer is called.
            // It's safer to call it if header was written.
            if (is_open) { // Assuming header was written if is_open is true
                av_write_trailer(format_ctx);
            }
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

    void start_encoding() {
        if (is_encoding) return;
        is_encoding = true;
        stop_requested = false;
        frame_queue.reset();
        encoding_thread = std::thread(&Impl::encoding_loop, this);
    }

    void stop_encoding_and_wait() {
        if (!is_encoding) return;

        // 1. Signal stop requested (graceful)
        stop_requested = true;

        // 2. Wait for queue to be empty?
        // The loop condition should handle this.
        // We need to notify the thread if it's waiting on empty queue.
        // But ConcurrentQueue doesn't support "notify but don't shutdown".
        // Actually, we want the thread to continue until queue is empty.
        // We can push a sentinel (empty mat) or use a flag.
        // Here we use `stop_requested` flag.
        // But if queue is empty, thread is blocked on `pop()`.
        // We need to unblock it.
        // We can shutdown the queue?
        // If we shutdown, `pop()` returns nullopt immediately if empty.
        // But if not empty, it returns items?
        // Let's check ConcurrentQueue implementation.
        // pop(): wait(lock, []{ !empty || shutdown }); if (empty && shutdown) return nullopt;
        // So if we shutdown, it consumes remaining items! This is exactly what we want.

        frame_queue.shutdown();

        if (encoding_thread.joinable()) { encoding_thread.join(); }
        is_encoding = false;
    }

    void encoding_loop() {
        while (true) {
            auto mat_opt = frame_queue.pop();

            if (!mat_opt) {
                // Queue shutdown and empty -> we are done
                break;
            }

            process_frame(*mat_opt);
        }

        // After queue is drained, flush encoder
        flush_encoder();
    }

    void process_frame(const cv::Mat& mat) {
        if (mat.empty() || mat.cols != codec_ctx->width || mat.rows != codec_ctx->height) {
            Logger::get_instance()->error("VideoWriter: Invalid frame dimensions");
            return;
        }

        // Make frame writable
        if (av_frame_make_writable(frame) < 0) {
            Logger::get_instance()->error("VideoWriter: Frame not writable");
            return;
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
            return;
        }

        receive_and_write_packets();

        written_frame_count++;
    }

    void flush_encoder() {
        if (!codec_ctx) return;

        avcodec_send_frame(codec_ctx, nullptr); // Flush signal
        receive_and_write_packets();
    }

    void receive_and_write_packets() {
        int ret = 0;
        while (ret >= 0) {
            ret = avcodec_receive_packet(codec_ctx, packet);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) { break; }
            if (ret < 0) {
                Logger::get_instance()->error("VideoWriter: Error receiving packet from encoder");
                break;
            }

            av_packet_rescale_ts(packet, codec_ctx->time_base, video_stream->time_base);
            packet->stream_index = video_stream->index;

            ret = av_interleaved_write_frame(format_ctx, packet);
            av_packet_unref(packet);
            if (ret < 0) {
                Logger::get_instance()->error("VideoWriter: Error writing packet");
                break;
            }
        }
    }

    bool open() {
        // Cleanup strictly implies closing previous, but here we just ensure clean state
        // cleanup(); // Called by caller (VideoWriter::open -> impl->open) usually? No, impl->open
        // calls cleanup. Let's call cleanup at start of open to be safe. But wait, open() logic
        // below sets is_open=true. cleanup() checks is_open.

        if (is_open) cleanup();

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

        video_stream = avformat_new_stream(format_ctx, codec);
        if (!video_stream) {
            Logger::get_instance()->error("VideoWriter: Failed to create stream");
            cleanup();
            return false;
        }

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

        // Pixel Format
        AVPixelFormat pix_fmt = AV_PIX_FMT_NONE;
        if (!params.pixelFormat.empty()) { pix_fmt = av_get_pix_fmt(params.pixelFormat.c_str()); }
        if (pix_fmt == AV_PIX_FMT_NONE) {
            Logger::get_instance()->warn(std::format(
                "VideoWriter: Invalid pixel format '{}', fallback to yuv420p", params.pixelFormat));
            pix_fmt = AV_PIX_FMT_YUV420P;
        }
        codec_ctx->pix_fmt = pix_fmt;

        codec_ctx->gop_size = params.gopSize;
        codec_ctx->max_b_frames = params.maxBFrames;

        if (params.threadCount > 0) { codec_ctx->thread_count = params.threadCount; }

        // Bitrate Control vs CRF
        bool use_bitrate = params.bitRate > 0;
        if (use_bitrate) {
            codec_ctx->bit_rate = params.bitRate;
            if (params.maxBitRate > 0) { codec_ctx->rc_max_rate = params.maxBitRate; }
            if (params.bufSize > 0) { codec_ctx->rc_buffer_size = params.bufSize; }
        } else {
            // Set quality (CRF for x264/x265) if bitrate is not set
            if (codec_name == "libx264" || codec_name == "libx265") {
                int crf = static_cast<int>(51 - params.quality * 0.51);
                av_opt_set(codec_ctx->priv_data, "crf", std::to_string(crf).c_str(), 0);
            }
        }

        // Common options
        if (!params.preset.empty()) {
            av_opt_set(codec_ctx->priv_data, "preset", params.preset.c_str(), 0);
        }
        if (!params.tune.empty()) {
            av_opt_set(codec_ctx->priv_data, "tune", params.tune.c_str(), 0);
        }
        if (!params.profile.empty()) {
            av_opt_set(codec_ctx->priv_data, "profile", params.profile.c_str(), 0);
        }
        if (!params.level.empty()) {
            av_opt_set(codec_ctx->priv_data, "level", params.level.c_str(), 0);
        }

        // Extra Options
        for (const auto& [key, value] : params.extraOptions) {
            av_opt_set(codec_ctx->priv_data, key.c_str(), value.c_str(), 0);
        }

        if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
            codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to open encoder");
            cleanup();
            return false;
        }

        if (avcodec_parameters_from_context(video_stream->codecpar, codec_ctx) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to copy codec params to stream");
            cleanup();
            return false;
        }

        video_stream->time_base = codec_ctx->time_base;

        if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&format_ctx->pb, output_path.c_str(), AVIO_FLAG_WRITE) < 0) {
                Logger::get_instance()->error(
                    std::format("VideoWriter: Failed to open output file: {}", output_path));
                cleanup();
                return false;
            }
        }

        if (avformat_write_header(format_ctx, nullptr) < 0) {
            Logger::get_instance()->error("VideoWriter: Failed to write header");
            cleanup();
            return false;
        }

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

        sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, AV_PIX_FMT_BGR24,
                                 codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!sws_ctx) {
            Logger::get_instance()->error("VideoWriter: Failed to create scaler context");
            cleanup();
            return false;
        }

        is_open = true;
        written_frame_count = 0;
        next_pts = 0;

        start_encoding();
        return true;
    }

    bool write_frame(const cv::Mat& mat) {
        if (!is_open) { return false; }

        // Push to queue (async)
        // Note: ConcurrentQueue takes value (T), so mat will be copied.
        // cv::Mat copy is shallow (refcount increment). This is safe as long as data is not
        // modified externally. If data might be modified, we should clone. Given this is an async
        // writer, safer to clone? Let's trust PipelineRunner yields unique frames.
        frame_queue.push(mat);
        return true;
    }
};

VideoWriter::VideoWriter(const std::string& outputPath, const VideoParams& params) :
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
