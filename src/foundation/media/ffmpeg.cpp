/**
 ******************************************************************************
 * @file           : ffmpeg.cpp
 * @brief          : FFmpeg runner module implementation
 ******************************************************************************
 */

module;
#include <numeric>
#include <fstream>
#include <unordered_set>
#include <ranges>
#include <iostream>
#include <format>
#include <vector>
#include <string>
#include <cmath>
#include <sstream>

#include <opencv2/opencv.hpp>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
}

module foundation.media.ffmpeg;
import foundation.infrastructure.logger;
import foundation.infrastructure.file_system;
import foundation.infrastructure.process;
import foundation.media.vision;

namespace foundation::media::ffmpeg {

using namespace foundation::infrastructure;
using namespace foundation::infrastructure::logger;

// Internal helper declarations (not exported)
std::string map_NVENC_preset(const std::string& preset);
std::string map_amf_preset(const std::string& preset);
std::string get_compression_and_preset_cmd(const unsigned int& quality, const std::string& preset,
                                           const std::string& codec);

std::vector<std::string> child_process(const std::string& command) {
    std::vector<std::string> lines;
    std::string output_buffer;
    try {
        std::string commandToRun = command;
#ifdef _WIN32
        commandToRun = file_system::utf8_to_sys_default_local(commandToRun);
#endif

        process::Process process(commandToRun, "", [&output_buffer](const char* bytes, size_t n) {
            output_buffer.append(bytes, n);
        });

        int exit_status = process.get_exit_status();

        // Process buffer similar to original implementation (splitting by whitespace/tokens
        // potentially, or just lines) Original code: pipeStream >> line. This does whitespace
        // splitting.
        std::stringstream ss(output_buffer);
        std::string temp;
        while (ss >> temp) { lines.push_back(temp); }

        if (exit_status != 0) {
            lines.emplace_back(
                std::format("Process exited with code {}, command: {}", exit_status, command));
        }
    } catch (const std::exception& e) {
        lines.emplace_back(std::format("Exception: {}", e.what()));
    }
    return lines;
}

bool is_video(const std::string& videoPath) {
    if (foundation::media::vision::is_image(videoPath)) { return false; }

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        Logger::get_instance()->error(
            std::format("{} : {}", __FUNCTION__, "Failed to open video file : " + videoPath));
        return false;
    }
    cap.release();
    return true;
}

bool is_audio(const std::string& audioPath) {
    if (!file_system::file_exists(audioPath)) {
        Logger::get_instance()->error(
            std::format("{} : {}", __FUNCTION__, "Not a audio file : " + audioPath));
        return false;
    }

    AVFormatContext* formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, audioPath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file." << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        return false;
    }

    for (size_t i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            avformat_close_input(&formatContext);
            return true;
        }
    }
    avformat_close_input(&formatContext);
    return false;
}

void extract_frames(const std::string& videoPath, const std::string& outputImagePattern) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error(std::format("{} : {}", __FUNCTION__, "Not a video file"));
        return;
    }

    if (!file_system::dir_exists(file_system::parent_path(outputImagePattern))) {
        file_system::create_dir(file_system::parent_path(outputImagePattern));
    }

    std::string command =
        "ffmpeg -v error -i \"" + videoPath + "\" -q:v 0 -vsync 0 \"" + outputImagePattern + "\"";
    std::vector<std::string> results = child_process(command);
    if (!results.empty()) {
        Logger::get_instance()->error(
            std::format("{} : {}", __FUNCTION__,
                        std::accumulate(results.begin(), results.end(), std::string())));
    }
}

bool cut_video_into_segments(const std::string& videoPath, const std::string& outputPath,
                             const unsigned int& segmentDuration,
                             const std::string& outputPattern) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error(
            std::format("{} : {}", __FUNCTION__, "Not a video file : " + videoPath));
        return false;
    }
    if (!file_system::dir_exists(outputPath)) { file_system::create_dir(outputPath); }

    const std::string durationStr = std::to_string(segmentDuration);
    const std::string command =
        "ffmpeg -v error -i \"" + videoPath + "\" -c:v copy -an -f segment -segment_time "
        + durationStr + " -reset_timestamps 1 -y \"" + outputPath + "/" + outputPattern + "\"";
    std::vector<std::string> results = child_process(command);
    if (!results.empty()) {
        Logger::get_instance()->error(
            std::format("{} : {}", __FUNCTION__,
                        std::accumulate(results.begin(), results.end(), std::string())));
        return false;
    }
    return true;
}

void extract_audios(const std::string& videoPath, const std::string& outputDir,
                    const Audio_Codec& audioCodec) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error("Not a video file : " + videoPath);
        return;
    }
    if (!file_system::dir_exists(outputDir)) { file_system::create_dir(outputDir); }

    std::string audioCodecStr;
    std::string extension;
    switch (audioCodec) {
    case Codec_UNKNOWN:
    case Audio_Codec::Codec_AAC:
        audioCodecStr = "aac";
        extension = ".aac";
        break;
    case Audio_Codec::Codec_MP3:
        audioCodecStr = "libmp3lame";
        extension = ".mp3";
        break;
    case Codec_OPUS:
        audioCodecStr = "libopus";
        extension = ".opus";
        break;
    case Codec_VORBIS:
        audioCodecStr = "libvorbis";
        extension = ".ogg";
        break;
    }

    for (const auto audioStreamsInfo = get_audio_streams_index_and_codec(videoPath);
         const auto& key : audioStreamsInfo | std::views::keys) {
        std::string index = std::to_string(key);
        std::string command = "ffmpeg -v error -i \"" + videoPath + "\" -map 0:" + index + " -c:a "
                            + audioCodecStr + " -vn -y \"" + outputDir + "/audio_" + index
                            + extension + "\"";
        if (std::vector<std::string> results = child_process(command); !results.empty()) {
            Logger::get_instance()->error(
                std::format("{} Failed to extract audio : {}", __FUNCTION__, command));
        }
    }
}

std::unordered_map<unsigned int, std::string> get_audio_streams_index_and_codec(
    const std::string& videoPath) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error("Not a video file : " + videoPath);
        return {};
    }

    AVFormatContext* formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, videoPath.c_str(), nullptr, nullptr) != 0) {
        std::cerr << "Could not open input file." << std::endl;
        return {};
    }

    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        return {};
    }

    std::unordered_map<unsigned int, std::string> results;
    for (size_t i = 0; i < formatContext->nb_streams; ++i) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            std::string codecName = avcodec_get_name(formatContext->streams[i]->codecpar->codec_id);
            results.emplace(static_cast<unsigned int>(i), codecName);
        }
    }
    avformat_close_input(&formatContext);
    return results;
}

bool concat_video_segments(const std::vector<std::string>& videoSegmentsPaths,
                           const std::string& outputVideoPath, const VideoPrams& videoPrams) {
    if (file_system::is_file(outputVideoPath) && file_system::file_exists(outputVideoPath)) {
        file_system::remove_file(outputVideoPath);
    }

    if (std::string parentPath = file_system::parent_path(outputVideoPath);
        file_system::is_dir(parentPath) && !file_system::dir_exists(parentPath)) {
        file_system::create_dir(parentPath);
    }

    std::string listVideoFilePath;
    std::string outputVideoBaseName = file_system::get_base_name(outputVideoPath);
    std::string listFileName = outputVideoBaseName + "_segments.txt";
    if (file_system::is_dir(outputVideoPath)) {
        listVideoFilePath = outputVideoPath + "/" + listFileName;
    } else {
        listVideoFilePath = file_system::parent_path(outputVideoPath) + "/" + listFileName;
    }
    std::ofstream listFile(listVideoFilePath);
    if (!listFile.is_open()) {
        Logger::get_instance()->error(std::format("{} : Failed to create list file", __FUNCTION__));
        return false;
    }

    for (const auto& videoSegmentPath : videoSegmentsPaths) {
        if (!is_video(videoSegmentPath)) {
            Logger::get_instance()->error(
                std::format("{} : {} is not a video file", __FUNCTION__, videoSegmentPath));
            return false;
        }
        listFile << "file '" << videoSegmentPath << "'" << std::endl;
    }
    listFile.close();

    std::string frameRate = std::to_string(videoPrams.frameRate);
    std::string outputRes =
        std::to_string(videoPrams.width) + "x" + std::to_string(videoPrams.height);

    std::string command;
    command = "ffmpeg -v error -f concat -safe 0 -r " + frameRate + " -i \"" + listVideoFilePath
            + "\" -s " + outputRes + " -c:v " + videoPrams.videoCodec + " ";
    command += get_compression_and_preset_cmd(videoPrams.quality, videoPrams.preset,
                                              videoPrams.videoCodec);
    if (file_system::is_dir(outputVideoPath)) {
        command += " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \""
                 + outputVideoPath + "/output.mp4\"";
    } else {
        command += " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \""
                 + outputVideoPath + "\"";
    }

    std::vector<std::string> results = child_process(command);
    file_system::remove_file(listVideoFilePath);
    if (!results.empty()) {
        std::string error = std::accumulate(results.begin(), results.end(), std::string());
        Logger::get_instance()->error("Failed to concat video segments! Error: " + error);
        return false;
    }
    return true;
}

std::unordered_set<std::string> filter_video_paths(
    const std::unordered_set<std::string>& filePaths) {
    std::unordered_set<std::string> filteredPaths;
    std::ranges::for_each(filePaths, [&](const std::string& videoPath) {
        if (is_video(videoPath)) { filteredPaths.insert(videoPath); }
    });
    return filteredPaths;
}

std::unordered_set<std::string> filter_audio_paths(
    const std::unordered_set<std::string>& filePaths) {
    std::unordered_set<std::string> filteredPaths;
    std::ranges::for_each(filePaths, [&](const std::string& audioPath) {
        if (is_audio(audioPath)) { filteredPaths.insert(audioPath); }
    });
    return filteredPaths;
}

bool add_audios_to_video(const std::string& videoPath, const std::vector<std::string>& audioPaths,
                         const std::string& outputVideoPath) {
    if (!is_video(videoPath)) {
        Logger::get_instance()->error("Not a video file : " + videoPath);
        return false;
    }
    if (file_system::is_dir(outputVideoPath)) {
        Logger::get_instance()->error("Output path is a directory : " + outputVideoPath);
        return false;
    }
    if (!file_system::dir_exists(file_system::parent_path(outputVideoPath))) {
        file_system::create_dir(file_system::parent_path(outputVideoPath));
    }

    if (audioPaths.empty()) {
        Logger::get_instance()->warn(std::format("{} No audio files to add", __FUNCTION__));
        file_system::copy(videoPath, outputVideoPath);
        return true;
    }

    std::string command = "ffmpeg -v error -i \"" + videoPath + "\"";
    for (const auto& audioPath : audioPaths) { command += " -i \"" + audioPath + "\""; }
    command += " -map 0:v:0";
    for (size_t i = 0; i < audioPaths.size(); ++i) {
        command += " -map " + std::to_string(i + 1) + ":a:0";
    }
    command += " -c:v copy -c:a copy -shortest -y \"" + outputVideoPath + "\"";
    if (const std::vector<std::string> results = child_process(command); !results.empty()) {
        Logger::get_instance()->error("Failed to add audios to video : " + command);
        return false;
    }
    return true;
}

bool images_to_video(const std::string& inputImagePattern, const std::string& outputVideoPath,
                     const VideoPrams& videoPrams) {
    if (inputImagePattern.empty() || outputVideoPath.empty()) {
        Logger::get_instance()->error(
            std::format("{} : inputImagePattern or outputVideoPath is empty", __FUNCTION__));
        return false;
    }

    if (file_system::is_dir(outputVideoPath)) {
        Logger::get_instance()->error(std::format("{} : Output video path is a directory : {}",
                                                  __FUNCTION__, outputVideoPath));
        return false;
    }
    if (file_system::is_file(outputVideoPath)) { file_system::remove_file(outputVideoPath); }
    if (!file_system::dir_exists(file_system::parent_path(outputVideoPath))) {
        file_system::create_dir(file_system::parent_path(outputVideoPath));
    }

    const std::string frameRate = std::to_string(videoPrams.frameRate);
    const std::string codec = videoPrams.videoCodec;
    const std::string outputRes =
        std::to_string(videoPrams.width) + "x" + std::to_string(videoPrams.height);

    std::string command = "ffmpeg -v error -r " + frameRate + " -i \"" + inputImagePattern
                        + "\" -s " + outputRes + " -c:v " + codec + " ";
    command += get_compression_and_preset_cmd(videoPrams.quality, videoPrams.preset, codec);
    command +=
        " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \"" + outputVideoPath + "\"";

    if (const std::vector<std::string> results = child_process(command); !results.empty()) {
        Logger::get_instance()->error("Failed to create video from images : " + command);
        Logger::get_instance()->error(
            std::accumulate(results.begin(), results.end(), std::string()));
        return false;
    }
    return true;
}

// Helper Implementations (not exported)

std::string map_NVENC_preset(const std::string& preset) {
    const std::unordered_set<std::string> fastPresets = {"ultrafast", "superfast", "veryfast",
                                                         "faster", "fast"};
    const std::unordered_set<std::string> mediumPresets = {"medium"};
    const std::unordered_set<std::string> slowPresets = {"slow", "slower", "veryslow"};

    if (fastPresets.contains(preset)) { return "fast"; }
    if (mediumPresets.contains(preset)) { return "medium"; }
    if (slowPresets.contains(preset)) { return "slow"; }
    Logger::get_instance()->warn(
        std::format("{} : Unknown preset: {}, using medium preset", __FUNCTION__, preset));
    return "medium";
}

std::string map_amf_preset(const std::string& preset) {
    const std::unordered_set<std::string> fastPresets = {"ultrafast", "superfast", "veryfast"};
    const std::unordered_set<std::string> mediumPresets = {"faster", "fast", "medium"};
    const std::unordered_set<std::string> slowPresets = {"slow", "slower", "veryslow"};

    if (fastPresets.contains(preset)) { return "speed"; }
    if (mediumPresets.contains(preset)) { return "balanced"; }
    if (slowPresets.contains(preset)) { return "quality"; }
    Logger::get_instance()->warn(
        std::format("{} : Unknown preset: {}, using medium preset", __FUNCTION__, preset));
    return "balanced";
}

std::string get_compression_and_preset_cmd(const unsigned int& quality, const std::string& preset,
                                           const std::string& codec) {
    if (codec == "libx264" || codec == "libx265") {
        const int crf = static_cast<int>(std::round(51 - static_cast<float>(quality * 0.51)));
        return "-crf " + std::to_string(crf) + " -preset " + preset;
    }
    if (codec == "libvpx-vp9") {
        const int crf = static_cast<int>(std::round(63 - static_cast<float>(quality * 0.63)));
        return "-cq " + std::to_string(crf);
    }
    if (codec == "h264_nvenc" || codec == "hevc_nvenc") {
        const int cq = static_cast<int>(std::round(51 - static_cast<float>(quality * 0.51)));
        return "-crf " + std::to_string(cq) + " -preset " + map_NVENC_preset(preset);
    }
    if (codec == "h264_amf" || codec == "hevc_amf") {
        const int qb_i = static_cast<int>(std::round(51 - static_cast<float>(quality * 0.51)));
        return "-qb_i " + std::to_string(qb_i) + " -qb_p " + std::to_string(qb_i) + " -quality "
             + map_amf_preset(preset);
    }
    return {};
}

Audio_Codec get_audio_codec(const std::string& codec) {
    if (codec == "aac") { return Audio_Codec::Codec_AAC; }
    if (codec == "mp3") { return Audio_Codec::Codec_MP3; }
    if (codec == "opus") { return Audio_Codec::Codec_OPUS; }
    if (codec == "vorbis") { return Audio_Codec::Codec_VORBIS; }

    Logger::get_instance()->warn(std::format("{} : Unknown audio codec: {}", __FUNCTION__, codec));
    return Audio_Codec::Codec_UNKNOWN;
}

VideoPrams::VideoPrams(const std::string& videoPath) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        Logger::get_instance()->error(
            std::format("{} : Failed to open video : {}", __FUNCTION__, videoPath));
        return;
    }
    width = static_cast<unsigned int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<unsigned int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    frameRate = cap.get(cv::CAP_PROP_FPS);
    quality = 80;
    cap.release();
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
