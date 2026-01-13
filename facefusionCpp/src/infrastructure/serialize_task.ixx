module;
#include <nlohmann/json.hpp>

export module serialize:task;

import task;
import processor_hub;
import core_options;
import face_masker_hub;
import logger;
import :options;
import types;

export namespace ffc::infra {

using namespace task;
using json = nlohmann::json;
using namespace core;

NLOHMANN_JSON_SERIALIZE_ENUM(ProcessorMajorType,
                             {
                                 {ProcessorMajorType::FaceSwapper, "face_swapper"},
                                 {ProcessorMajorType::FrameEnhancer, "frame_enhancer"},
                                 {ProcessorMajorType::ExpressionRestorer, "expression_restorer"},
                                 {ProcessorMajorType::FaceEnhancer, "face_enhancer"},
                             });

void to_json(json& j, Task::Output& output) {
    j = {{"path", output.path}, {"prefix", output.prefix}, {"subfix", output.subfix}};
}

void from_json(json& j, Task::Output& output) {
    j.at("path").get_to(output.path);
    j.at("prefix").get_to(output.prefix);
    j.at("subfix").get_to(output.subfix);
}

void to_json(json& j, Task::ProcessorInfo& info) {
    j = json{
        {"type", info.type},
        {"model", info.model},
        {"parameters", info.parameters},
    };
}

void from_json(json& j, Task::ProcessorInfo& info) {
    j.at("type").get_to(info.type);
    if (info.type == ProcessorMajorType::ExpressionRestorer) {
        if (j.contains("model") && j.at("model") == "live_portrait") {
            info.model = model_manager::Model::Unknown;
        }
    } else {
        j.at("model").get_to(info.model);
    }
    if (info.type != ProcessorMajorType::FaceSwapper && j.contains("parameters")) {
        for (auto& [key, value] : j.at("parameters").items()) {
            info.parameters[key] = value.dump();
        }
    }
}

NLOHMANN_JSON_SERIALIZE_ENUM(ImageFormat, {
                                              {ImageFormat::JPEG, "jpeg"},
                                              {ImageFormat::PNG, "png"},
                                              {ImageFormat::JPG, "jpg"},
                                          });

NLOHMANN_JSON_SERIALIZE_ENUM(VideoEncoder, {
                                               {VideoEncoder::libx264, "libx264"},
                                               {VideoEncoder::libx265, "libx265"},
                                               {VideoEncoder::libvpx_vp9, "libvpx-vp9"},
                                               {VideoEncoder::h264_nvenc, "h264-nvenc"},
                                               {VideoEncoder::hevc_nvenc, "h265-nvenc"},
                                               {VideoEncoder::h264_amf, "h264_amf"},
                                               {VideoEncoder::hevc_nvenc, "hevc_nvenc"},
                                           });

NLOHMANN_JSON_SERIALIZE_ENUM(VideoPreset, {
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

NLOHMANN_JSON_SERIALIZE_ENUM(AudioEncoder, {
                                               {AudioEncoder::aac, "aac"},
                                               {AudioEncoder::libmp3lame, "libmp3lame"},
                                               {AudioEncoder::libopus, "libopus"},
                                               {AudioEncoder::libvorbis, "libvorbis"},
                                           })

void to_json(json& j, Task::ImageInfo& info) {
    json size_json;
    to_json(size_json, info.output_resolution);
    j = json{
        {"output_quality", info.output_quality},
        {"output_resolution", size_json},
        {"output_format", info.output_format},
    };
}

void from_json(json& j, Task::ImageInfo& info) {
    j.at("output_quality").get_to<unsigned short>(info.output_quality);
    from_json(j.at("output_resolution"), info.output_resolution);
    j.at("output_format").get_to(info.output_format);
}

void to_json(json& j, Task::VideoInfo& info) {
    j = json{
        {"segment_duration", info.segment_duration}, {"video_encoder", info.video_encoder},
        {"video_preset", info.video_preset},         {"video_quality", info.video_quality},
        {"audio_encoder", info.audio_encoder},       {"skip_audio", info.skip_audio},
        {"frame_format", info.frame_format},
    };
}

void from_json(json& j, Task::VideoInfo& info) {
    j.at("segment_duration").get_to(info.segment_duration);
    j.at("video_encoder").get_to(info.video_encoder);
    j.at("video_preset").get_to(info.video_preset);
    j.at("video_quality").get_to(info.video_quality);
    j.at("audio_encoder").get_to(info.audio_encoder);
    j.at("skip_audio").get_to(info.skip_audio);
    j.at("frame_format").get_to(info.frame_format);
}

void to_json(json& j, Task::FaceMasker& face_masker) {
    json regions_json;
    to_json(regions_json, face_masker.mask_regions);
    j = json{
        {"occluder_model", face_masker.occluder_model}, {"parser_model", face_masker.parser_model},
        {"mask_type", face_masker.mask_type},           {"mask_blur", face_masker.mask_blur},
        {"mask_padding", face_masker.mask_padding},     {"mask_regions", regions_json},
    };
}

void from_json(json& j, Task::FaceMasker& face_masker) {
    if (j.contains("occluder_model")) {
        from_json(j.at("occluder_model"), face_masker.occluder_model);
    }
    if (j.contains("parser_model")) { from_json(j.at("parser_model"), face_masker.parser_model); }
    if (j.contains("mask_type")) { from_json(j.at("mask_type"), face_masker.mask_type); }
    if (j.contains("mask_blur")) { from_json(j.at("mask_blur"), face_masker.mask_blur); }
    if (j.contains("mask_padding")) { from_json(j.at("mask_padding"), face_masker.mask_padding); }
    if (j.at("mask_regions").is_array() == true) {
        if (j.at("mask_regions").contains("all")) {
            face_masker.mask_regions = FaceMaskerRegion::getAllRegions();
        } else {
            from_json(j.at("mask_regions"), face_masker.mask_regions);
        }
    } else {
        from_json(j.at("mask_regions"), face_masker.mask_regions);
    }
}

void to_json(json& j, Task& task) {
    json output_json, processors_info_json, image_info_json, video_info_json, face_analyser_json,
        face_masker_json;
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

} // namespace ffc::infra
