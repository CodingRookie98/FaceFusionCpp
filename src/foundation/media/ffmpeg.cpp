/**
 ******************************************************************************
 * @file           : ffmpeg.cpp
 * @brief          : FFmpeg runner module implementation
 ******************************************************************************
 */

module;
#include <string>
#include <vector>
#include <iostream>
#include <format>
#include <cstdio>
#include <filesystem>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

module foundation.media.ffmpeg;
import foundation.infrastructure.logger;
import foundation.infrastructure.file_system;
import foundation.media.vision;

namespace foundation::media::ffmpeg {

using namespace foundation::infrastructure;
using namespace foundation::infrastructure::logger;

bool is_video(const std::string& videoPath) {
    if (foundation::media::vision::is_image(videoPath)) { return false; }

    AVFormatContext* format_ctx = nullptr;
    // Suppress logs for probing
    int original_level = av_log_get_level();
    av_log_set_level(AV_LOG_QUIET);

    int ret = avformat_open_input(&format_ctx, videoPath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        av_log_set_level(original_level);
        return false;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
        avformat_close_input(&format_ctx);
        av_log_set_level(original_level);
        return false;
    }

    bool has_video = false;
    for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
        if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            has_video = true;
            break;
        }
    }

    avformat_close_input(&format_ctx);
    av_log_set_level(original_level);
    return has_video;
}

void extract_frames(const std::string& videoPath, const std::string& outputImagePattern) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error(
            std::format("{} : Not a video file or open failed: {}", __FUNCTION__, videoPath));
        return;
    }

    namespace fs = std::filesystem;
    fs::path patternPath(outputImagePattern);
    fs::path parentDir = patternPath.parent_path();
    if (!parentDir.empty() && !fs::exists(parentDir)) { fs::create_directories(parentDir); }

    VideoReader reader(videoPath);
    if (!reader.open()) {
        Logger::get_instance()->error("Failed to open video for frame extraction");
        return;
    }

    int frame_index = 1;
    cv::Mat frame;

    // Check if pattern contains format specifier
    bool has_format = outputImagePattern.find("%") != std::string::npos;

    while (true) {
        frame = reader.read_frame();
        if (frame.empty()) break;

        std::string filename;
        if (has_format) {
            char buffer[1024];
            // Safe snprintf - handling potential format issues is redundant if we assume valid
            // input
            std::snprintf(buffer, sizeof(buffer), outputImagePattern.c_str(), frame_index);
            filename = buffer;
        } else {
            // Append index if no format specifier (fallback)
            filename = outputImagePattern + "_" + std::to_string(frame_index) + ".png";
        }

        if (!cv::imwrite(filename, frame)) {
            Logger::get_instance()->warn(std::format("Failed to write frame: {}", filename));
        }
        frame_index++;
    }
}

bool compose_video_from_images(const std::string& inputImagePattern,
                               const std::string& outputVideoPath, const VideoPrams& videoPrams) {
    // Determine input directory and verify basics
    // OpenCV VideoCapture supports patterns like "path/img_%04d.jpg"

    cv::VideoCapture cap(inputImagePattern);
    // Try to open to check if pattern is valid or matches files
    if (!cap.isOpened()) {
        Logger::get_instance()->error(std::format("{} : Failed to open input images sequence: {}",
                                                  __FUNCTION__, inputImagePattern));
        return false;
    }

    namespace fs = std::filesystem;
    fs::path outPath(outputVideoPath);
    if (outPath.has_parent_path() && !fs::exists(outPath.parent_path())) {
        fs::create_directories(outPath.parent_path());
    }

    // Adjust params if needed based on input?
    // Usually we trust videoPrams provided by Caller, but we might want to check dimensions from
    // first frame. However, VideoWriter needs fixed dimensions.

    // We can read first frame to check dims if videoPrams are default?
    // Let's assume caller provides correct params matching the images.

    VideoWriter writer(outputVideoPath, videoPrams);
    if (!writer.open()) {
        Logger::get_instance()->error(
            std::format("{} : Failed to open video writer: {}", __FUNCTION__, outputVideoPath));
        return false;
    }

    cv::Mat frame;
    while (true) {
        if (!cap.read(frame)) break;
        if (frame.empty()) break;

        // Resize if doesn't match params? VideoWriter::write_frame requires exact match.
        // Doing resize here for robustness
        if (static_cast<unsigned int>(frame.cols) != videoPrams.width
            || static_cast<unsigned int>(frame.rows) != videoPrams.height) {
            cv::resize(frame, frame, cv::Size(videoPrams.width, videoPrams.height));
        }

        if (!writer.write_frame(frame)) {
            Logger::get_instance()->error("Failed to write frame to video");
            break;
        }
    }

    return true;
}

VideoPrams::VideoPrams(const std::string& videoPath) {
    if (videoPath.empty()) {
        return; // default values
    }

    AVFormatContext* format_ctx = nullptr;
    int original_level = av_log_get_level();
    av_log_set_level(AV_LOG_QUIET);

    if (avformat_open_input(&format_ctx, videoPath.c_str(), nullptr, nullptr) < 0) {
        av_log_set_level(original_level);
        Logger::get_instance()->error(
            std::format("{} : Failed to open video : {}", __FUNCTION__, videoPath));
        return;
    }

    if (avformat_find_stream_info(format_ctx, nullptr) >= 0) {
        for (unsigned int i = 0; i < format_ctx->nb_streams; i++) {
            if (format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                AVStream* stream = format_ctx->streams[i];
                width = stream->codecpar->width;
                height = stream->codecpar->height;

                // FPS calculation similar to VideoReader
                double fps_val = av_q2d(stream->avg_frame_rate);
                if (fps_val < 0.1 || fps_val > 200.0) {
                    double r_fps = av_q2d(stream->r_frame_rate);
                    if (r_fps >= 0.1 && r_fps <= 200.0) fps_val = r_fps;
                    else fps_val = 30.0;
                }
                frameRate = fps_val;
                break;
            }
        }
    }

    avformat_close_input(&format_ctx);
    av_log_set_level(original_level);
}

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

        // Open input file
        if (avformat_open_input(&format_ctx, video_path.c_str(), nullptr, nullptr) < 0) {
            Logger::get_instance()->error(
                std::format("VideoReader: Failed to open input file: {}", video_path));
            return false;
        }

        // Get stream info
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

        // Find decoder
        const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
        if (!codec) {
            Logger::get_instance()->error("VideoReader: Decoder not found");
            cleanup();
            return false;
        }

        // Create codec context
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

        // Open codec
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
            Logger::get_instance()->error("VideoReader: Failed to open codec");
            cleanup();
            return false;
        }

        // Allocate frames
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

        // Create scaler context
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
