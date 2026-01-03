/**
 * @file serialize.ixx
 * @brief Serialization module for JSON serialization/deserialization
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides JSON serialization functionality using nlohmann-json library
 */
module;
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

export module serialize;

import processor_hub;
import task;
import types;
import face_detector_hub;
import face_landmarker_hub;
import face_analyser;
import face_selector;
import face_recognizer_hub;
import face_masker_hub;
import core_options;
import logger;
import model_manager;

export namespace ffc::infra {

using namespace ffc::face_detector;
using namespace ffc::face_landmarker;
using json = nlohmann::json;
using namespace ffc::face_masker;
using namespace ffc::core;
using namespace task;

/**
 * @brief Serialize Model enum to JSON
 * @note This macro enables JSON serialization for model_manager::Model enum
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    model_manager::Model,
    {
        {model_manager::Model::Gfpgan_12, "gfpgan_1.2"},
        {model_manager::Model::Gfpgan_13, "gfpgan_1.3"},
        {model_manager::Model::Gfpgan_14, "gfpgan_1.4"},
        {model_manager::Model::Codeformer, "codeformer"},
        {model_manager::Model::Inswapper_128, "inswapper_128"},
        {model_manager::Model::Inswapper_128_fp16, "inswapper_128_fp16"},
        {model_manager::Model::Face_detector_retinaface, "face_detector_retinaface"},
        {model_manager::Model::Face_detector_scrfd, "face_detector_scrfd"},
        {model_manager::Model::Face_detector_yoloface, "face_detector_yoloface"},
        {model_manager::Model::Face_recognizer_arcface_w600k_r50, "face_recognizer_arcface_w600k_r50"},
        {model_manager::Model::Face_landmarker_68, "face_landmarker_68"},
        {model_manager::Model::Face_landmarker_peppawutz, "face_landmarker_peppa_wutz"},
        {model_manager::Model::Face_landmarker_68_5, "face_landmarker_68_5"},
        {model_manager::Model::FairFace, "fairface"},
        {model_manager::Model::bisenet_resnet_18, "bisenet_resnet_18"},
        {model_manager::Model::bisenet_resnet_34, "bisenet_resnet_34"},
        {model_manager::Model::xseg_1, "xseg_1"},
        {model_manager::Model::xseg_2, "xseg_2"},
        {model_manager::Model::Feature_extractor, "feature_extractor"},
        {model_manager::Model::Motion_extractor, "motion_extractor"},
        {model_manager::Model::Generator, "generator"},
        {model_manager::Model::Real_esrgan_x2, "real_esrgan_x2"},
        {model_manager::Model::Real_esrgan_x2_fp16, "real_esrgan_x2_fp16"},
        {model_manager::Model::Real_esrgan_x4, "real_esrgan_x4"},
        {model_manager::Model::Real_esrgan_x4_fp16, "real_esrgan_x4_fp16"},
        {model_manager::Model::Real_esrgan_x8, "real_esrgan_x8"},
        {model_manager::Model::Real_esrgan_x8_fp16, "real_esrgan_x8_fp16"},
        {model_manager::Model::Real_hatgan_x4, "real_hatgan_x4"},
    });

/**
 * @brief Serialize ModelInfo to JSON
 * @param j JSON object to write to
 * @param model_info ModelInfo object to serialize
 */
void to_json(json& j, model_manager::ModelInfo& model_info) {
    j = json{
        {"name", model_info.name},
        {"path", model_info.path},
        {"url", model_info.url},
    };
}

/**
 * @brief Deserialize ModelInfo from JSON
 * @param j JSON object to read from
 * @param model_info ModelInfo object to deserialize
 */
void from_json(json& j, model_manager::ModelInfo& model_info) {
    model_info.name = j.value("name", model_info.name);
    model_info.path = j.value("path", model_info.path);
    model_info.url = j.value("url", model_info.url);
    from_json(model_info.name, model_info.model);
}

/**
 * @brief Serialize FaceDetectorHub::Type enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    face_detector::FaceDetectorHub::Type,
    {
        {face_detector::FaceDetectorHub::Type::Yolo, "yolo"},
        {face_detector::FaceDetectorHub::Type::Retina, "retina"},
        {face_detector::FaceDetectorHub::Type::Scrfd, "scrfd"},
    });

/**
 * @brief Serialize cv::Size to JSON
 * @param j JSON object to write to
 * @param v cv::Size object to serialize
 */
void to_json(json& j, cv::Size& v) {
    j = json{
        {"width", v.width},
        {"height", v.width},
    };
}

/**
 * @brief Deserialize cv::Size from JSON
 * @param j JSON object to read from
 * @param v cv::Size object to deserialize
 */
void from_json(json& j, cv::Size& v) {
    j.at("width").get_to(v.width);
    j.at("height").get_to(v.height);
}

/**
 * @brief Serialize unordered_set of FaceDetectorHub::Type to JSON
 * @param j JSON object to write to
 * @param v Set of FaceDetectorHub::Type to serialize
 */
void to_json(json& j, std::unordered_set<FaceDetectorHub::Type>& v) {
    for (auto& type : v) {
        json j_type;
        to_json(j_type, type);
        j.push_back(j_type);
    }
}

/**
 * @brief Deserialize unordered_set of FaceDetectorHub::Type from JSON
 * @param j JSON object to read from
 * @param v Set of FaceDetectorHub::Type to deserialize
 */
void from_json(json& j, std::unordered_set<FaceDetectorHub::Type>& v) {
    for (auto& j_type : j) {
        FaceDetectorHub::Type type;
        from_json(j_type, type);
        v.insert(type);
    }
}

/**
 * @brief Serialize FaceDetectorHub::Options to JSON
 * @param j JSON object to write to
 * @param options FaceDetectorHub::Options object to serialize
 */
void to_json(json& j, FaceDetectorHub::Options& options) {
    json sz_json, types_json;
    to_json(sz_json, options.size);
    to_json(types_json, options.types);
    j = json{
        {"size", sz_json},
        {"models", types_json},
        {"angle", options.angle},
        {"min_score", options.min_score},
    };
}

/**
 * @brief Deserialize FaceDetectorHub::Options from JSON
 * @param j JSON object to read from
 * @param options FaceDetectorHub::Options object to deserialize
 */
void from_json(json& j, FaceDetectorHub::Options& options) {
    from_json(j.at("size"), options.size);
    from_json(j.at("models"), options.types);
    // j.at("angle").get_to(options.angle);
    j.at("min_score").get_to(options.min_score);
}

/**
 * @brief Serialize FaceLandmarkerHub::Type enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    face_landmarker::FaceLandmarkerHub::Type,
    {
        {face_landmarker::FaceLandmarkerHub::Type::_2DFAN, "2dfan4"},
        {face_landmarker::FaceLandmarkerHub::Type::PEPPA_WUTZ, "peppa_wutz"},
    });

/**
 * @brief Serialize unordered_set of FaceLandmarkerHub::Type to JSON
 * @param j JSON object to write to
 * @param v Set of FaceLandmarkerHub::Type to serialize
 */
void to_json(json& j, std::unordered_set<FaceLandmarkerHub::Type>& v) {
    for (auto& type : v) {
        json j_type;
        to_json(j_type, type);
        j.push_back(j_type);
    }
}

/**
 * @brief Deserialize unordered_set of FaceLandmarkerHub::Type from JSON
 * @param j JSON object to read from
 * @param v Set of FaceLandmarkerHub::Type to deserialize
 */
void from_json(json& j, std::unordered_set<FaceLandmarkerHub::Type>& v) {
    for (auto& j_type : j) {
        FaceLandmarkerHub::Type type;
        from_json(j_type, type);
        v.insert(type);
    }
}

/**
 * @brief Serialize FaceLandmarkerHub::Options to JSON
 * @param j JSON object to write to
 * @param options FaceLandmarkerHub::Options object to serialize
 */
void to_json(json& j, FaceLandmarkerHub::Options& options) {
    json types_json;
    to_json(types_json, options.types);
    j = json{
        {"models", types_json},
        {"angle", options.angle},
        {"min_score", options.minScore},
    };
}

/**
 * @brief Deserialize FaceLandmarkerHub::Options from JSON
 * @param j JSON object to read from
 * @param options FaceLandmarkerHub::Options object to deserialize
 */
void from_json(json& j, FaceLandmarkerHub::Options& options) {
    from_json(j.at("models"), options.types);
    // j.at("angle").get_to(options.angle);
    j.at("min_score").get_to(options.minScore);
}

/**
 * @brief Serialize FaceSelector::FaceSelectorOrder enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    FaceSelector::FaceSelectorOrder,
    {
        {FaceSelector::FaceSelectorOrder::Left_Right, "left_right"},
        {FaceSelector::FaceSelectorOrder::Right_Left, "right_left"},
        {FaceSelector::FaceSelectorOrder::Top_Bottom, "top_bottom"},
        {FaceSelector::FaceSelectorOrder::Bottom_Top, "bottom_top"},
        {FaceSelector::FaceSelectorOrder::Small_Large, "small_large"},
        {FaceSelector::FaceSelectorOrder::Large_Small, "large_small"},
        {FaceSelector::FaceSelectorOrder::Best_Worst, "best_worst"},
        {FaceSelector::FaceSelectorOrder::Worst_Best, "worst_best"},
    });

/**
 * @brief Serialize Gender enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    Gender,
    {
        {Gender::Male, "male"},
        {Gender::Female, "female"},
    });

/**
 * @brief Serialize Race enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    Race,
    {
        {Race::Black, "black"},
        {Race::Latino, "latino"},
        {Race::Indian, "indian"},
        {Race::Asian, "asian"},
        {Race::Arabic, "arabic"},
        {Race::White, "white"},
    });

/**
 * @brief Serialize FaceSelector::Options to JSON
 * @param j JSON object to write to
 * @param options FaceSelector::Options object to serialize
 */
void to_json(json& j, FaceSelector::Options& options) {
    j = json{
        {"order", options.order},
        {"gender", options.genders},
        {"race", options.races},
        {"age_start", options.age_start},
        {"age_end", options.age_end},
    };
}

/**
 * @brief Deserialize FaceSelector::Options from JSON
 * @param j JSON object to read from
 * @param options FaceSelector::Options object to deserialize
 */
void from_json(json& j, FaceSelector::Options& options) {
    j.at("order").get_to(options.order);
    j.at("gender").get_to(options.genders);
    j.at("race").get_to(options.races);
    j.at("age_start").is_null() ? options.age_start : j.at("age_start").get_to<unsigned int>(options.age_start);
    j.at("age_end").is_null() ? options.age_end : j.at("age_end").get_to<unsigned int>(options.age_end);
}

/**
 * @brief Serialize FaceAnalyser::Options to JSON
 * @param j JSON object to write to
 * @param options FaceAnalyser::Options object to serialize
 */
void to_json(nlohmann::json& j, FaceAnalyser::Options& options) {
    json fd_json, fl_json, fs_json;
    to_json(fd_json, options.faceDetectorOptions);
    to_json(fl_json, options.faceLandMarkerOptions);
    to_json(fs_json, options.faceSelectorOptions);
    j = json{
        {"face_detector", fd_json},
        {"face_landmarker", fl_json},
        {"face_selector", fs_json},
    };
}

/**
 * @brief Deserialize FaceAnalyser::Options from JSON
 * @param j JSON object to read from
 * @param options FaceAnalyser::Options object to deserialize
 */
void from_json(json& j, FaceAnalyser::Options& options) {
    from_json(j.at("face_detector"), options.faceDetectorOptions);
    from_json(j.at("face_landmarker"), options.faceLandMarkerOptions);
    from_json(j.at("face_selector"), options.faceSelectorOptions);
}

/**
 * @brief Serialize ProcessorMajorType enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    ProcessorMajorType,
    {
        {ProcessorMajorType::FaceSwapper, "face_swapper"},
        {ProcessorMajorType::FrameEnhancer, "frame_enhancer"},
        {ProcessorMajorType::ExpressionRestorer, "expression_restorer"},
        {ProcessorMajorType::FaceEnhancer, "face_enhancer"},
    });

/**
 * @brief Serialize Task::Output to JSON
 * @param j JSON object to write to
 * @param output Task::Output object to serialize
 */
void to_json(json& j, Task::Output& output) {
    j = {
        {"path", output.path},
        {"prefix", output.prefix},
        {"subfix", output.subfix}};
}

/**
 * @brief Deserialize Task::Output from JSON
 * @param j JSON object to read from
 * @param output Task::Output object to deserialize
 */
void from_json(json& j, Task::Output& output) {
    j.at("path").get_to(output.path);
    j.at("prefix").get_to(output.prefix);
    j.at("subfix").get_to(output.subfix);
}

/**
 * @brief Serialize Task::ProcessorInfo to JSON
 * @param j JSON object to write to
 * @param info Task::ProcessorInfo object to serialize
 */
void to_json(json& j, Task::ProcessorInfo& info) {
    j = json{
        {"type", info.type},
        {"model", info.model},
        {"parameters", info.parameters},
    };
}

/**
 * @brief Deserialize Task::ProcessorInfo from JSON
 * @param j JSON object to read from
 * @param info Task::ProcessorInfo object to deserialize
 * @note Special handling for ExpressionRestorer with live_portrait model
 */
void from_json(json& j, Task::ProcessorInfo& info) {
    j.at("type").get_to(info.type);
    if (info.type == ProcessorMajorType::ExpressionRestorer) {
        if (j.contains("model") && j.at("model") == "live_portrait") {
            // Because model names are different, special handling is required here
            // live_portrait model name is unknown, because live_portrait uses three models
            info.model = model_manager::Model::Unknown;
        }
    } else {
        j.at("model").get_to(info.model);
    }
    if (info.type != ProcessorMajorType::FaceSwapper && j.contains("parameters")) {
        // parameters is a json array, each element is a key-value pair
        for (auto& [key, value] : j.at("parameters").items()) {
            info.parameters[key] = value.dump();
        }
    }
}

/**
 * @brief Serialize ImageFormat enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    ImageFormat,
    {
        {ImageFormat::JPEG, "jpeg"},
        {ImageFormat::PNG, "png"},
        {ImageFormat::JPG, "jpg"},
    });

/**
 * @brief Serialize VideoEncoder enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    VideoEncoder,
    {
        {VideoEncoder::libx264, "libx264"},
        {VideoEncoder::libx265, "libx265"},
        {VideoEncoder::libvpx_vp9, "libvpx-vp9"},
        {VideoEncoder::h264_nvenc, "h264-nvenc"},
        {VideoEncoder::hevc_nvenc, "h265-nvenc"},
        {VideoEncoder::h264_amf, "h264_amf"},
        {VideoEncoder::hevc_nvenc, "hevc_nvenc"},
    });

/**
 * @brief Serialize VideoPreset enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    VideoPreset,
    {
        {VideoPreset::ultrafast, "ultrafast"},
        {VideoPreset::superfast, "superfast"},
        {VideoPreset::veryfast, "veryfast"},
        {VideoPreset::faster, "faster"},
        {VideoPreset::fast, "fast"},
        {VideoPreset::medium, "medium"},
        {VideoPreset::slow, "slow"},
        {VideoPreset::slower, "slower"},
        {VideoPreset::veryslow, "veryslow"},
    });

/**
 * @brief Serialize AudioEncoder enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    AudioEncoder,
    {
        {AudioEncoder::aac, "aac"},
        {AudioEncoder::libmp3lame, "libmp3lame"},
        {AudioEncoder::libopus, "libopus"},
        {AudioEncoder::libvorbis, "libvorbis"},
    })

/**
 * @brief Serialize Task::ImageInfo to JSON
 * @param j JSON object to write to
 * @param info Task::ImageInfo object to serialize
 */
void to_json(json& j, Task::ImageInfo& info) {
    json size_json;
    to_json(size_json, info.output_resolution);
    j = json{
        {"output_quality", info.output_quality},
        {"output_resolution", size_json},
        {"output_format", info.output_format},
    };
}

/**
 * @brief Deserialize Task::ImageInfo from JSON
 * @param j JSON object to read from
 * @param info Task::ImageInfo object to deserialize
 */
void from_json(json& j, Task::ImageInfo& info) {
    j.at("output_quality").get_to<unsigned short>(info.output_quality);
    from_json(j.at("output_resolution"), info.output_resolution);
    j.at("output_format").get_to(info.output_format);
}

/**
 * @brief Serialize Task::VideoInfo to JSON
 * @param j JSON object to write to
 * @param info Task::VideoInfo object to serialize
 */
void to_json(json& j, Task::VideoInfo& info) {
    j = json{
        {"segment_duration", info.segment_duration},
        {"video_encoder", info.video_encoder},
        {"video_preset", info.video_preset},
        {"video_quality", info.video_quality},
        {"audio_encoder", info.audio_encoder},
        {"skip_audio", info.skip_audio},
        {"frame_format", info.frame_format},
    };
}

/**
 * @brief Deserialize Task::VideoInfo from JSON
 * @param j JSON object to read from
 * @param info Task::VideoInfo object to deserialize
 */
void from_json(json& j, Task::VideoInfo& info) {
    j.at("segment_duration").get_to(info.segment_duration);
    j.at("video_encoder").get_to(info.video_encoder);
    j.at("video_preset").get_to(info.video_preset);
    j.at("video_quality").get_to(info.video_quality);
    j.at("audio_encoder").get_to(info.audio_encoder);
    j.at("skip_audio").get_to(info.skip_audio);
    j.at("frame_format").get_to(info.frame_format);
}

/**
 * @brief Serialize FaceMaskerHub::Type enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    face_masker::FaceMaskerHub::Type,
    {
        {face_masker::FaceMaskerHub::Type::Box, "box"},
        {face_masker::FaceMaskerHub::Type::Occlusion, "occlusion"},
        {face_masker::FaceMaskerHub::Type::Region, "region"},
    });

/**
 * @brief Serialize FaceMaskerRegion::Region enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    face_masker::FaceMaskerRegion::Region,
    {
        {face_masker::FaceMaskerRegion::Region::Skin, "skin"},
        {face_masker::FaceMaskerRegion::Region::LeftEyebrow, "left_eyebrow"},
        {face_masker::FaceMaskerRegion::Region::RightEyebrow, "right_eyebrow"},
        {face_masker::FaceMaskerRegion::Region::LeftEye, "left_eye"},
        {face_masker::FaceMaskerRegion::Region::RightEye, "right_eye"},
        {face_masker::FaceMaskerRegion::Region::Glasses, "glasses"},
        {face_masker::FaceMaskerRegion::Region::Nose, "nose"},
        {face_masker::FaceMaskerRegion::Region::Mouth, "mouth"},
        {face_masker::FaceMaskerRegion::Region::UpperLip, "upper_lip"},
        {face_masker::FaceMaskerRegion::Region::LowerLip, "lower_lip"},
    });

/**
 * @brief Serialize unordered_set of FaceMaskerRegion::Region to JSON
 * @param j JSON object to write to
 * @param regions Set of FaceMaskerRegion::Region to serialize
 */
void to_json(json& j, std::unordered_set<FaceMaskerRegion::Region>& regions) {
    j = json::array();
    for (auto region : regions) {
        json region_json;
        to_json(region_json, region);
        j.push_back(region_json);
    }
}

/**
 * @brief Deserialize unordered_set of FaceMaskerRegion::Region from JSON
 * @param j JSON object to read from
 * @param regions Set of FaceMaskerRegion::Region to deserialize
 */
void from_json(json& j, std::unordered_set<FaceMaskerRegion::Region>& regions) {
    for (auto& region_json : j) {
        FaceMaskerRegion::Region region;
        from_json(region_json, region);
        regions.insert(region);
    }
}

/**
 * @brief Serialize Task::FaceMasker to JSON
 * @param j JSON object to write to
 * @param face_masker Task::FaceMasker object to serialize
 */
void to_json(json& j, Task::FaceMasker& face_masker) {
    json regions_json;
    to_json(regions_json, face_masker.mask_regions);
    j = json{
        {"occluder_model", face_masker.occluder_model},
        {"parser_model", face_masker.parser_model},
        {"mask_type", face_masker.mask_type},
        {"mask_blur", face_masker.mask_blur},
        {"mask_padding", face_masker.mask_padding},
        {"mask_regions", regions_json},
    };
}

/**
 * @brief Deserialize Task::FaceMasker from JSON
 * @param j JSON object to read from
 * @param face_masker Task::FaceMasker object to deserialize
 * @note Special handling for "all" regions which expands to all available regions
 */
void from_json(json& j, Task::FaceMasker& face_masker) {
    if (j.contains("occluder_model")) {
        from_json(j.at("occluder_model"), face_masker.occluder_model);
    }
    if (j.contains("parser_model")) {
        from_json(j.at("parser_model"), face_masker.parser_model);
    }
    if (j.contains("mask_type")) {
        from_json(j.at("mask_type"), face_masker.mask_type);
    }
    if (j.contains("mask_blur")) {
        from_json(j.at("mask_blur"), face_masker.mask_blur);
    }
    if (j.contains("mask_padding")) {
        from_json(j.at("mask_padding"), face_masker.mask_padding);
    }
    if (j.at("mask_regions").is_array() == true) {
        // If contains all region, parse as all regions
        if (j.at("mask_regions").contains("all")) {
            face_masker.mask_regions = FaceMaskerRegion::getAllRegions();
        } else {
            from_json(j.at("mask_regions"), face_masker.mask_regions);
        }
    } else {
        from_json(j.at("mask_regions"), face_masker.mask_regions);
    }
}

/**
 * @brief Serialize Task to JSON
 * @param j JSON object to write to
 * @param task Task object to serialize
 */
void to_json(json& j, Task& task) {
    json output_json, processors_info_json, image_info_json, video_info_json, face_analyser_json, face_masker_json;
    to_json(output_json, task.output);
    for (auto& processor_info : task.processors_info) {
        json processor_info_json;
        to_json(processor_info_json, processor_info);
        processors_info_json.push_back(processor_info_json);
    }
    to_json(image_info_json, task.image);
    to_json(video_info_json, task.video);
    to_json(face_analyser_json, task.face_analyser);
    to_json(face_masker_json, task.face_masker);
    j = nlohmann::json{
        {"source_paths", task.source_paths},
        {"target_paths", task.target_paths},
        {"output", output_json},
        {"processor_list", processors_info_json},
        {"image", image_info_json},
        {"video", video_info_json},
        {"face_analyser", face_analyser_json},
        {"face_masker", face_masker_json},
    };
}

/**
 * @brief Deserialize Task from JSON
 * @param j JSON object to read from
 * @param task Task object to deserialize
 */
void from_json(json& j, Task& task) {
    from_json(j.at("source_paths"), task.source_paths);
    from_json(j.at("target_paths"), task.target_paths);
    from_json(j.at("output"), task.output);
    for (auto& processor_info_json : j.at("processor_list")) {
        Task::ProcessorInfo processor_info;
        from_json(processor_info_json, processor_info);
        task.processors_info.push_back(processor_info);
    }
    from_json(j.at("image"), task.image);
    from_json(j.at("video"), task.video);
    from_json(j.at("face_analyser"), task.face_analyser);
    from_json(j.at("face_masker"), task.face_masker);
}

/**
 * @brief Serialize CoreOptions::ModelOptions to JSON
 * @param j JSON object to write to
 * @param model_options CoreOptions::ModelOptions object to serialize
 */
void to_json(json& j, CoreOptions::ModelOptions& model_options) {
    j = json{
        {"force_download", model_options.force_download},
        {"skip_download", model_options.skip_download},
    };
}

/**
 * @brief Deserialize CoreOptions::ModelOptions from JSON
 * @param j JSON object to read from
 * @param model_options CoreOptions::ModelOptions object to deserialize
 */
void from_json(json& j, CoreOptions::ModelOptions& model_options) {
    j.at("force_download").get_to(model_options.force_download);
    j.at("skip_download").get_to(model_options.skip_download);
}

/**
 * @brief Serialize Logger::LogLevel enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    Logger::LogLevel,
    {
        {Logger::LogLevel::Trace, "trace"},
        {Logger::LogLevel::Debug, "debug"},
        {Logger::LogLevel::Info, "info"},
        {Logger::LogLevel::Warn, "warn"},
        {Logger::LogLevel::Error, "error"},
        {Logger::LogLevel::Critical, "critical"},
    })

/**
 * @brief Serialize CoreOptions::LoggerOptions to JSON
 * @param j JSON object to write to
 * @param logger_options CoreOptions::LoggerOptions object to serialize
 */
void to_json(json& j, CoreOptions::LoggerOptions& logger_options) {
    json log_level_json;
    to_json(log_level_json, logger_options.log_level);
    j = json{
        {"log_level", log_level_json},
    };
}

/**
 * @brief Deserialize CoreOptions::LoggerOptions from JSON
 * @param j JSON object to read from
 * @param logger_options CoreOptions::LoggerOptions object to deserialize
 */
void from_json(json& j, CoreOptions::LoggerOptions& logger_options) {
    from_json(j.at("log_level"), logger_options.log_level);
}

/**
 * @brief Serialize CoreOptions::TaskOptions to JSON
 * @param j JSON object to write to
 * @param task_options CoreOptions::TaskOptions object to serialize
 */
void to_json(json& j, CoreOptions::TaskOptions& task_options) {
    j = json{
        {"per_task_thread_count", task_options.per_task_thread_count},
    };
}

/**
 * @brief Deserialize CoreOptions::TaskOptions from JSON
 * @param j JSON object to read from
 * @param task_options CoreOptions::TaskOptions object to deserialize
 */
void from_json(json& j, CoreOptions::TaskOptions& task_options) {
    j.at("per_task_thread_count").get_to(task_options.per_task_thread_count);
}

/**
 * @brief Serialize InferenceSession::ExecutionProvider enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    InferenceSession::ExecutionProvider,
    {
        {InferenceSession::ExecutionProvider::CPU, "cpu"},
        {InferenceSession::ExecutionProvider::CUDA, "cuda"},
        {InferenceSession::ExecutionProvider::TensorRT, "tensor_rt"},
    });

/**
 * @brief Serialize unordered_set of InferenceSession::ExecutionProvider to JSON
 * @param j JSON object to write to
 * @param execution_providers Set of InferenceSession::ExecutionProvider to serialize
 */
void to_json(json& j, std::unordered_set<InferenceSession::ExecutionProvider>& execution_providers) {
    for (auto provider : execution_providers) {
        json provider_json;
        to_json(provider_json, provider);
        j.push_back(provider_json);
    }
}

/**
 * @brief Deserialize unordered_set of InferenceSession::ExecutionProvider from JSON
 * @param j JSON object to read from
 * @param execution_providers Set of InferenceSession::ExecutionProvider to deserialize
 */
void from_json(json& j, std::unordered_set<InferenceSession::ExecutionProvider>& execution_providers) {
    for (auto& provider_json : j) {
        InferenceSession::ExecutionProvider provider;
        from_json(provider_json, provider);
        execution_providers.insert(provider);
    }
}

/**
 * @brief Serialize CoreOptions::MemoryStrategy enum to JSON
 */
NLOHMANN_JSON_SERIALIZE_ENUM(
    CoreOptions::MemoryStrategy,
    {
        {CoreOptions::MemoryStrategy::Strict, "strict"},
        {CoreOptions::MemoryStrategy::Tolerant, "tolerant"},
    });

/**
 * @brief Serialize CoreOptions::MemoryOptions to JSON
 * @param j JSON object to write to
 * @param memory_options CoreOptions::MemoryOptions object to serialize
 */
void to_json(json& j, CoreOptions::MemoryOptions& memory_options) {
    j = json{
        {"processor_memory_strategy", memory_options.processor_memory_strategy},
    };
}

/**
 * @brief Deserialize CoreOptions::MemoryOptions from JSON
 * @param j JSON object to read from
 * @param memory_options CoreOptions::MemoryOptions object to deserialize
 */
void from_json(json& j, CoreOptions::MemoryOptions& memory_options) {
    memory_options.processor_memory_strategy = j.value("processor_memory_strategy", memory_options.processor_memory_strategy);
}

/**
 * @brief Serialize InferenceSession::Options to JSON
 * @param j JSON object to write to
 * @param inference_session_options InferenceSession::Options object to serialize
 * @note TensorRT workspace size is converted from bytes to GB
 */
void to_json(json& j, InferenceSession::Options& inference_session_options) {
    j = json{
        {"device_id", inference_session_options.execution_device_id},
        {"providers", inference_session_options.execution_providers},
        {"enable_engine_cache", inference_session_options.enable_tensorrt_cache},
        {"enable_embed_engine", inference_session_options.enable_tensorrt_embed_engine},
        {"trt_max_workspace_size", inference_session_options.trt_max_workspace_size / (2 << 30)},
    };
}

/**
 * @brief Deserialize InferenceSession::Options from JSON
 * @param j JSON object to read from
 * @param inference_session_options InferenceSession::Options object to deserialize
 * @note TensorRT workspace size is converted from GB to bytes
 */
void from_json(json& j, InferenceSession::Options& inference_session_options) {
    j.at("device_id").get_to(inference_session_options.execution_device_id);
    from_json(j.at("providers"), inference_session_options.execution_providers);
    j.at("enable_engine_cache").get_to(inference_session_options.enable_tensorrt_cache);
    j.at("enable_embed_engine").get_to(inference_session_options.enable_tensorrt_embed_engine);
    j.at("trt_max_workspace_size").get_to(inference_session_options.trt_max_workspace_size);
    inference_session_options.trt_max_workspace_size *= 2 << 30;
}

void to_json(json& j, CoreOptions& core_options) {
    json model_json, logger_json, task_json, memory_json, inference_session_json;
    to_json(model_json, core_options.model_options);
    to_json(logger_json, core_options.logger_options);
    to_json(task_json, core_options.task_options);
    to_json(memory_json, core_options.memory_options);
    to_json(inference_session_json, core_options.inference_session_options);
    j = json{
        {"model", model_json},
        {"log", logger_json},
        {"task", task_json},
        {"memory", memory_json},
        {"inference_session", inference_session_json},
    };
}
void from_json(json& j, CoreOptions& core_options) {
    from_json(j.at("model"), core_options.model_options);
    from_json(j.at("log"), core_options.logger_options);
    from_json(j.at("task"), core_options.task_options);
    from_json(j.at("memory"), core_options.memory_options);
    from_json(j.at("inference_session"), core_options.inference_session_options);
}
} // namespace ffc::infra