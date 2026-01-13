/**
 ******************************************************************************
 * @file           : job.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/13
 ******************************************************************************
 */
module;
#include <string>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

export module task;

import utils;
import processor_hub;
import types;
import face_detector_hub;
import face_landmarker_hub;
import face_analyser;
import face_masker_hub;
import logger;

namespace ffc::task {

using namespace infra;
using namespace ai;
using namespace core;

using Json = nlohmann::json;

export class Task {
public:
    std::vector<std::string> source_paths;
    std::vector<std::string> target_paths;
    struct Output {
        std::string path, prefix, subfix;
    };
    Output output;

    struct ProcessorInfo {
        ProcessorMajorType type{ProcessorMajorType::FaceSwapper};
        model_manager::Model model{model_manager::Model::Inswapper_128};
        std::unordered_map<std::string, std::string> parameters;
    };
    std::vector<ProcessorInfo> processors_info;

    struct ImageInfo {
        unsigned short output_quality{100};
        cv::Size output_resolution{0, 0};
        ImageFormat output_format{ImageFormat::PNG};
    };
    ImageInfo image;

    struct VideoInfo {
        unsigned int segment_duration{0};
        VideoEncoder video_encoder{
            VideoEncoder::libx264}; // 指定视频编码器。默认: libx264, 选择: libx264 libx265
                                    // libvpx-vp9 h264_nvenc hevc_nvenc h264_amf hevc_amf, 示例:
                                    // libx265
        VideoPreset video_preset{
            VideoPreset::veryslow}; // 输出视频编码器预设。默认: veryslow, 选择: ultrafast superfast
                                    // veryfast faster fast medium slow slower veryslow, 示例:
                                    // faster
        unsigned short video_quality{
            100}; // 设置输出的视频质量。默认: 80, 范围: 0 to 100 at 1, 示例: 60
        AudioEncoder audio_encoder{
            AudioEncoder::aac}; // 设置音频编码器。默认: aac, 选择: aac libmp3lame libopus
                                // libvorbis, 示例: libmp3lame
        bool skip_audio{false}; // 设置是否跳过音频处理。默认: false, 选择: true false, 示例: true
        ImageFormat frame_format{
            ImageFormat::PNG}; // 设置提取的视频帧格式。默认: png, 选择: png jpg bmp
    };
    VideoInfo video;

    FaceAnalyser::Options face_analyser;

    struct FaceMasker {
        model_manager::Model occluder_model{model_manager::Model::xseg_1};
        model_manager::Model parser_model{model_manager::Model::bisenet_resnet_34};
        face_masker::FaceMaskerHub::Type mask_type{face_masker::FaceMaskerHub::Type::Box};
        float mask_blur{0.3f};
        std::array<int, 4> mask_padding{0, 0, 0, 0};
        std::unordered_set<face_masker::FaceMaskerRegion::Region> mask_regions{
            face_masker::FaceMaskerRegion::getAllRegions()};
    };
    FaceMasker face_masker;
};

export void prepare_task(Task& task) {
    auto logger = Logger::get_instance();
    if (task.target_paths.empty()) {
        logger->error("[Task] No target path.");
        return;
    }
}
} // namespace ffc::task