/**
 ******************************************************************************
 * @file           : ffmpeg_runner.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-8-6
 ******************************************************************************
 */

module;
#include <numeric>
#include <fstream>
#include <unordered_set>
#include <ranges>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
// #include <libavutil/channel_layout.h>
// #include <libavutil/common.h>
// #include <libavutil/frame.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}
#include <boost/process.hpp>
#include <opencv2/opencv.hpp>

module ffmpeg_runner;
import logger;
import file_system;

namespace ffc {
namespace bp = boost::process;

std::vector<std::string> FfmpegRunner::childProcess(const std::string &command) {
    std::vector<std::string> lines;
    try {
        // 使用 Boost.Process 启动进程并获取其输出
        std::string commandToRun = command;

        // 使用 Boost.Process 启动进程
        bp::ipstream pipeStream; // 用于读取进程输出
#ifdef _WIN32
        commandToRun = FileSystem::utf8ToSysDefaultLocal(commandToRun);
        bp::child c(commandToRun, bp::std_out > pipeStream);
#else
        bp::child c(commandToRun, bp::std_out > pipeStream);
#endif

        if (!c.valid()) {
            std::string error = "child process is valid : " + commandToRun;
            lines.emplace_back(error);
            return lines;
        }

        // 读取输出
        std::string line;
        while (pipeStream >> line) {
            lines.push_back(line);
        }
        // 等待进程结束
        c.wait();

        if (c.exit_code() != 0) {
            lines.emplace_back(std::format("Process exited with code {}, command: {}", c.exit_code(), command));
        }
    } catch (const std::exception &e) {
        lines.emplace_back(std::format("Exception: {}", e.what()));
    }

    return lines;
}

bool FfmpegRunner::isVideo(const std::string &videoPath) {
    if (FileSystem::isImage(videoPath)) {
        return false;
    }

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, "Failed to open video file : " + videoPath));
        return false;
    }
    cap.release();
    return true;
}

bool FfmpegRunner::isAudio(const std::string &audioPath) {
    if (!FileSystem::fileExists(audioPath)) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, "Not a audio file : " + audioPath));
        return false;
    }

    AVFormatContext *formatContext = avformat_alloc_context();
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

void FfmpegRunner::extractFrames(const std::string &videoPath, const std::string &outputImagePattern) {
    if (!isVideo(videoPath)) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, "Not a video file"));
        return;
    }

    if (!FileSystem::dirExists(FileSystem::parentPath(outputImagePattern))) {
        FileSystem::createDir(FileSystem::parentPath(outputImagePattern));
    }

    std::string command = "ffmpeg -v error -i \"" + videoPath + "\" -q:v 0 -vsync 0 \"" + outputImagePattern + "\"";
    std::vector<std::string> results = childProcess(command);
    if (!results.empty()) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, std::accumulate(results.begin(), results.end(), std::string())));
    }
}

bool FfmpegRunner::cutVideoIntoSegments(const std::string &videoPath, const std::string &outputPath,
                                        const unsigned int &segmentDuration, const std::string &outputPattern) {
    if (!isVideo(videoPath)) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, "Not a video file : " + videoPath));
        return false;
    }
    if (!FileSystem::dirExists(outputPath)) {
        FileSystem::createDir(outputPath);
    }

    const std::string durationStr = std::to_string(segmentDuration);
    const std::string command = "ffmpeg -v error -i \"" + videoPath + "\" -c:v copy -an -f segment -segment_time " + durationStr + " -reset_timestamps 1 -y \"" + outputPath + "/" + outputPattern + "\"";
    std::vector<std::string> results = childProcess(command);
    if (!results.empty()) {
        Logger::getInstance()->error(std::format("{} : {}", __FUNCTION__, std::accumulate(results.begin(), results.end(), std::string())));
        return false;
    }
    return true;
}

void FfmpegRunner::extractAudios(const std::string &videoPath, const std::string &outputDir,
                                 const FfmpegRunner::Audio_Codec &audioCodec) {
    if (!isVideo(videoPath)) {
        Logger::getInstance()->error("Not a video file : " + videoPath);
        return;
    }
    if (!FileSystem::dirExists(outputDir)) {
        FileSystem::createDir(outputDir);
    }

    std::string audioCodecStr;
    std::string extension;
    switch (audioCodec) {
    case Codec_UNKNOWN:
    case FfmpegRunner::Audio_Codec::Codec_AAC:
        audioCodecStr = "aac";
        extension = ".aac";
        break;
    case FfmpegRunner::Audio_Codec::Codec_MP3:
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

    for (const auto audioStreamsInfo = getAudioStreamsIndexAndCodec(videoPath);
         const auto &key : audioStreamsInfo | std::views::keys) {
        std::string index = std::to_string(key);
        std::string command = "ffmpeg -v error -i \"" + videoPath + "\" -map 0:" + index + " -c:a " + audioCodecStr + " -vn -y \"" + outputDir + "/audio_" + index + extension + "\"";
        if (std::vector<std::string> results = childProcess(command); !results.empty()) {
            Logger::getInstance()->error(std::format("{} Failed to extract audio : {}", __FUNCTION__, command));
        }
    }
}

std::unordered_map<unsigned int, std::string> FfmpegRunner::getAudioStreamsIndexAndCodec(const std::string &videoPath) {
    if (!isVideo(videoPath)) {
        Logger::getInstance()->error("Not a video file : " + videoPath);
        return {};
    }

    AVFormatContext *formatContext = avformat_alloc_context();
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
            results.emplace(i, codecName);
        }
    }
    avformat_close_input(&formatContext);

    return results;
}

bool FfmpegRunner::concatVideoSegments(const std::vector<std::string> &videoSegmentsPaths,
                                       const std::string &outputVideoPath, const VideoPrams &videoPrams) {
    if (FileSystem::isFile(outputVideoPath) && FileSystem::fileExists(outputVideoPath)) {
        FileSystem::removeFile(outputVideoPath);
    }

    if (std::string parentPath = FileSystem::parentPath(outputVideoPath);
        FileSystem::isDir(parentPath) && !FileSystem::dirExists(parentPath)) {
        FileSystem::createDir(parentPath);
    }

    std::string listVideoFilePath;
    // get outputVideo base name
    std::string outputVideoBaseName = FileSystem::getBaseName(outputVideoPath);
    std::string listFileName = outputVideoBaseName + "_segments.txt";
    if (FileSystem::isDir(outputVideoPath)) {
        listVideoFilePath = outputVideoPath + "/" + listFileName;
    } else {
        listVideoFilePath = FileSystem::parentPath(outputVideoPath) + "/" + listFileName;
    }
    std::ofstream listFile(listVideoFilePath);
    if (!listFile.is_open()) {
        Logger::getInstance()->error(std::format("{} : Failed to create list file", __FUNCTION__));
        return false;
    }

    for (const auto &videoSegmentPath : videoSegmentsPaths) {
        if (!isVideo(videoSegmentPath)) {
            Logger::getInstance()->error(std::format("{} : {} is not a video file", __FUNCTION__, videoSegmentPath));
            return false;
        }
        listFile << "file '" << videoSegmentPath << "'" << std::endl;
    }
    listFile.close();
    std::string frameRate = std::to_string(videoPrams.frameRate);
    std::string outputRes = std::to_string(videoPrams.width) + "x" + std::to_string(videoPrams.height);

    std::string command;
    command = "ffmpeg -v error -f concat -safe 0 -r " + frameRate + " -i \"" + listVideoFilePath + "\" -s " + outputRes + " -c:v " + videoPrams.videoCodec + " ";
    command += getCompressionAndPresetCmd(videoPrams.quality, videoPrams.preset, videoPrams.videoCodec);
    if (FileSystem::isDir(outputVideoPath)) {
        command += " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \"" + outputVideoPath + "/output.mp4\"";
    } else {
        command += " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \"" + outputVideoPath + "\"";
    }

    std::vector<std::string> results = childProcess(command);
    FileSystem::removeFile(listVideoFilePath);
    if (!results.empty()) {
        std::string error = std::accumulate(results.begin(), results.end(), std::string());
        Logger::getInstance()->error("Failed to concat video segments! Error: " + error);
        return false;
    }
    return true;
}

std::unordered_set<std::string> FfmpegRunner::filterVideoPaths(const std::unordered_set<std::string> &filePaths) {
    std::unordered_set<std::string> filteredPaths;
    std::ranges::for_each(filePaths, [&](const std::string &videoPath) {
        if (isVideo(videoPath)) {
            filteredPaths.insert(videoPath);
        }
    });
    return filteredPaths;
}

std::unordered_set<std::string> FfmpegRunner::filterAudioPaths(const std::unordered_set<std::string> &filePaths) {
    std::unordered_set<std::string> filteredPaths;
    std::ranges::for_each(filePaths, [&](const std::string &audioPath) {
        if (isAudio(audioPath)) {
            filteredPaths.insert(audioPath);
        }
    });
    return filteredPaths;
}

bool FfmpegRunner::addAudiosToVideo(const std::string &videoPath,
                                    const std::vector<std::string> &audioPaths,
                                    const std::string &outputVideoPath) {
    if (!isVideo(videoPath)) {
        Logger::getInstance()->error("Not a video file : " + videoPath);
        return false;
    }
    if (FileSystem::isDir(outputVideoPath)) {
        Logger::getInstance()->error("Output path is a directory : " + outputVideoPath);
        return false;
    }
    if (!FileSystem::dirExists(FileSystem::parentPath(outputVideoPath))) {
        FileSystem::createDir(FileSystem::parentPath(outputVideoPath));
    }

    if (audioPaths.empty()) {
        Logger::getInstance()->warn(std::format("{} No audio files to add", __FUNCTION__));
        FileSystem::copy(videoPath, outputVideoPath);
        return true;
    }

    std::string command = "ffmpeg -v error -i \"" + videoPath + "\"";
    for (const auto &audioPath : audioPaths) {
        command += " -i \"" + audioPath + "\"";
    }
    command += " -map 0:v:0";
    for (size_t i = 0; i < audioPaths.size(); ++i) {
        command += " -map " + std::to_string(i + 1) + ":a:0";
    }
    command += " -c:v copy -c:a copy -shortest -y \"" + outputVideoPath + "\"";
    if (const std::vector<std::string> results = childProcess(command); !results.empty()) {
        Logger::getInstance()->error("Failed to add audios to video : " + command);
        return false;
    }
    return true;
}

bool FfmpegRunner::imagesToVideo(const std::string &inputImagePattern,
                                 const std::string &outputVideoPath,
                                 const FfmpegRunner::VideoPrams &videoPrams) {
    if (inputImagePattern.empty() || outputVideoPath.empty()) {
        Logger::getInstance()->error(std::format("{} : inputImagePattern or outputVideoPath is empty", __FUNCTION__));
        return false;
    }

    if (FileSystem::isDir(outputVideoPath)) {
        Logger::getInstance()->error(std::format("{} : Output video path is a directory : {}", __FUNCTION__, outputVideoPath));
        return false;
    }
    if (FileSystem::isFile(outputVideoPath)) {
        FileSystem::removeFile(outputVideoPath);
    }
    if (!FileSystem::dirExists(FileSystem::parentPath(outputVideoPath))) {
        FileSystem::createDir(FileSystem::parentPath(outputVideoPath));
    }

    const std::string frameRate = std::to_string(videoPrams.frameRate);
    const std::string codec = videoPrams.videoCodec;
    const std::string outputRes = std::to_string(videoPrams.width) + "x" + std::to_string(videoPrams.height);

    std::string command = "ffmpeg -v error -r " + frameRate + " -i \"" + inputImagePattern + "\" -s " + outputRes + " -c:v " + codec + " ";
    command += getCompressionAndPresetCmd(videoPrams.quality, videoPrams.preset, codec);
    command += " -pix_fmt yuv420p -colorspace bt709 -y -r " + frameRate + " \"" + outputVideoPath + "\"";

    if (const std::vector<std::string> results = childProcess(command); !results.empty()) {
        Logger::getInstance()->error("Failed to create video from images : " + command);
        Logger::getInstance()->error(std::accumulate(results.begin(), results.end(), std::string()));
        return false;
    }
    return true;
}

std::string FfmpegRunner::map_NVENC_preset(const std::string &preset) {
    const std::unordered_set<std::string> fastPresets = {"ultrafast", "superfast", "veryfast", "faster", "fast"};
    const std::unordered_set<std::string> mediumPresets = {"medium"};
    const std::unordered_set<std::string> slowPresets = {"slow", "slower", "veryslow"};

    if (fastPresets.contains(preset)) {
        return "fast";
    }
    if (mediumPresets.contains(preset)) {
        return "medium";
    }
    if (slowPresets.contains(preset)) {
        return "slow";
    }
    Logger::getInstance()->warn(std::format("{} : Unknown preset: {}, using medium preset", __FUNCTION__, preset));
    return "medium";
}

std::string FfmpegRunner::map_amf_preset(const std::string &preset) {
    const std::unordered_set<std::string> fastPresets = {"ultrafast", "superfast", "veryfast"};
    const std::unordered_set<std::string> mediumPresets = {"faster", "fast", "medium"};
    const std::unordered_set<std::string> slowPresets = {"slow", "slower", "veryslow"};

    if (fastPresets.contains(preset)) {
        return "speed";
    }
    if (mediumPresets.contains(preset)) {
        return "balanced";
    }
    if (slowPresets.contains(preset)) {
        return "quality";
    }
    Logger::getInstance()->warn(std::format("{} : Unknown preset: {}, using medium preset", __FUNCTION__, preset));
    return "balanced";
}

std::string FfmpegRunner::getCompressionAndPresetCmd(const unsigned int &quality, const std::string &preset, const std::string &codec) {
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
        return "-qb_i " + std::to_string(qb_i) + " -qb_p " + std::to_string(qb_i) + " -quality " + map_amf_preset(preset);
    }
    return {};
}

FfmpegRunner::Audio_Codec FfmpegRunner::getAudioCodec(const std::string &codec) {
    if (codec == "aac") {
        return Audio_Codec::Codec_AAC;
    }
    if (codec == "mp3") {
        return Audio_Codec::Codec_MP3;
    }
    if (codec == "opus") {
        return Audio_Codec::Codec_OPUS;
    }
    if (codec == "vorbis") {
        return Audio_Codec::Codec_VORBIS;
    }

    Logger::getInstance()->warn(std::format("{} : Unknown audio codec: {}", __FUNCTION__, codec));
    return Audio_Codec::Codec_UNKNOWN;
}

FfmpegRunner::VideoPrams::VideoPrams(const std::string &videoPath) {
    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        Logger::getInstance()->error(std::format("{} : Failed to open video : {}", __FUNCTION__, videoPath));
        return;
    }
    width = static_cast<unsigned int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    height = static_cast<unsigned int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    frameRate = cap.get(cv::CAP_PROP_FPS);
    quality = 80;
    cap.release();
}
} // namespace ffc