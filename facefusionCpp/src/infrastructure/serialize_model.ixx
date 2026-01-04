/**
 * @file serialize_model.ixx
 * @brief Model serialization module for JSON serialization/deserialization
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides model-related JSON serialization functionality using nlohmann-json library
 */
module;
#include <nlohmann/json.hpp>

export module serialize:model;

import model_manager;

export namespace ffc::infra {

using json = nlohmann::json;
using namespace ai;

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

} // namespace ffc::infra
