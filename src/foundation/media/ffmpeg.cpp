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

namespace {
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ptr) const {
        if (ptr) { avformat_close_input(&ptr); }
    }
};
using FormatCtxPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
} // namespace

bool is_video(const std::string& videoPath) {
    if (foundation::media::vision::is_image(videoPath)) { return false; }

    AVFormatContext* raw_ctx = nullptr;
    // Suppress logs for probing
    int original_level = av_log_get_level();
    av_log_set_level(AV_LOG_QUIET);

    int ret = avformat_open_input(&raw_ctx, videoPath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        av_log_set_level(original_level);
        return false;
    }
    FormatCtxPtr format_ctx(raw_ctx);

    if (avformat_find_stream_info(format_ctx.get(), nullptr) < 0) {
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
                               const std::string& outputVideoPath, const VideoParams& videoParams) {
    // OpenCV VideoCapture supports patterns like "path/img_%04d.jpg"

    cv::VideoCapture cap(inputImagePattern);
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

    // TODO: Validate or adjust params based on first frame dimensions if needed

    VideoWriter writer(outputVideoPath, videoParams);
    if (!writer.open()) {
        Logger::get_instance()->error(
            std::format("{} : Failed to open video writer: {}", __FUNCTION__, outputVideoPath));
        return false;
    }

    cv::Mat frame;
    while (true) {
        if (!cap.read(frame)) break;
        if (frame.empty()) break;

        // Resize if doesn't match params (VideoWriter requires exact match)
        if (static_cast<unsigned int>(frame.cols) != videoParams.width
            || static_cast<unsigned int>(frame.rows) != videoParams.height) {
            cv::resize(frame, frame, cv::Size(videoParams.width, videoParams.height));
        }

        if (!writer.write_frame(frame)) {
            Logger::get_instance()->error("Failed to write frame to video");
            break;
        }
    }

    return true;
}

VideoParams::VideoParams(const std::string& videoPath) {
    if (videoPath.empty()) {
        return; // default values
    }

    AVFormatContext* raw_ctx = nullptr;
    int original_level = av_log_get_level();
    av_log_set_level(AV_LOG_QUIET);

    if (avformat_open_input(&raw_ctx, videoPath.c_str(), nullptr, nullptr) < 0) {
        av_log_set_level(original_level);
        Logger::get_instance()->error(
            std::format("{} : Failed to open video : {}", __FUNCTION__, videoPath));
        return;
    }
    FormatCtxPtr format_ctx(raw_ctx);

    if (avformat_find_stream_info(format_ctx.get(), nullptr) >= 0) {
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

    av_log_set_level(original_level);
}

} // namespace foundation::media::ffmpeg
