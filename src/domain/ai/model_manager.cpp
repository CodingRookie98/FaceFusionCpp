/**
 ******************************************************************************
 * @file           : model_manager.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <fstream>
#include <nlohmann/json.hpp>

module domain.ai.model_manager;
import foundation.infrastructure.file_system;
import foundation.infrastructure.network;
import foundation.infrastructure.logger;

namespace domain::ai::model_manager {
using namespace foundation::infrastructure;

using json = nlohmann::json;

// Manual mapping for Model enum to string
static const std::unordered_map<Model, std::string> model_to_string_map = {
    {Model::Gfpgan_12, "gfpgan_1.2"},
    {Model::Gfpgan_13, "gfpgan_1.3"},
    {Model::Gfpgan_14, "gfpgan_1.4"},
    {Model::Codeformer, "codeformer"},
    {Model::Inswapper_128, "inswapper_128"},
    {Model::Inswapper_128_fp16, "inswapper_128_fp16"},
    {Model::Face_detector_retinaface, "face_detector_retinaface"},
    {Model::Face_detector_scrfd, "face_detector_scrfd"},
    {Model::Face_detector_yoloface, "face_detector_yoloface"},
    {Model::Face_recognizer_arcface_w600k_r50, "face_recognizer_arcface_w600k_r50"},
    {Model::Face_landmarker_68, "face_landmarker_68"},
    {Model::Face_landmarker_peppawutz, "face_landmarker_peppa_wutz"},
    {Model::Face_landmarker_68_5, "face_landmarker_68_5"},
    {Model::FairFace, "fairface"},
    {Model::bisenet_resnet_18, "bisenet_resnet_18"},
    {Model::bisenet_resnet_34, "bisenet_resnet_34"},
    {Model::xseg_1, "xseg_1"},
    {Model::xseg_2, "xseg_2"},
    {Model::Feature_extractor, "feature_extractor"},
    {Model::Motion_extractor, "motion_extractor"},
    {Model::Generator, "generator"},
    {Model::Real_esrgan_x2, "real_esrgan_x2"},
    {Model::Real_esrgan_x2_fp16, "real_esrgan_x2_fp16"},
    {Model::Real_esrgan_x4, "real_esrgan_x4"},
    {Model::Real_esrgan_x4_fp16, "real_esrgan_x4_fp16"},
    {Model::Real_esrgan_x8, "real_esrgan_x8"},
    {Model::Real_esrgan_x8_fp16, "real_esrgan_x8_fp16"},
    {Model::Real_hatgan_x4, "real_hatgan_x4"},
    {Model::Unknown, "unknown"}
};

static const std::unordered_map<std::string, Model> string_to_model_map = [] {
    std::unordered_map<std::string, Model> map;
    for (const auto& pair : model_to_string_map) {
        map[pair.second] = pair.first;
    }
    return map;
}();

void to_json(json& j, const Model& model) {
    if (model_to_string_map.contains(model)) {
        j = model_to_string_map.at(model);
    } else {
        j = "unknown";
    }
}

void from_json(const json& j, Model& model) {
    std::string s = j.get<std::string>();
    if (string_to_model_map.contains(s)) {
        model = string_to_model_map.at(s);
    } else {
        model = Model::Unknown;
    }
}

void to_json(json& j, const ModelInfo& model_info) {
    j = json{
        {"name", model_info.name},
        {"path", model_info.path},
        {"url", model_info.url},
    };
}

void from_json(const json& j, ModelInfo& model_info) {
    model_info.name = j.value("name", model_info.name);
    model_info.path = j.value("path", model_info.path);
    model_info.url = j.value("url", model_info.url);
    if (j.contains("name")) {
        // The 'name' field in the JSON is used to derive the Model enum value.
        // We convert the string 'name' to a temporary json object to call the existing from_json(const json&, Model&)
        json temp_json_for_model = model_info.name;
        from_json(temp_json_for_model, model_info.model);
    }
}

ModelManager::ModelManager(const std::string& json_file_path) {
    m_json_file_path = json_file_path;
    json models_info_json;
    if (std::ifstream file(m_json_file_path); file.is_open()) {
        models_info_json = nlohmann::json::parse(file);
        file.close();
    } else {
        throw std::runtime_error("Failed to open " + m_json_file_path);
    }
    if (auto model_info_json = models_info_json.items().begin().value(); model_info_json.is_array()) {
        for (const auto& model_info_json_item : model_info_json.items()) {
            ModelInfo model_info;
            from_json(model_info_json_item.value(), model_info);
            m_models_info_map[model_info.model] = model_info;
        }
    }
}

std::string ModelManager::get_model_name(const Model& model) {
    if (m_models_info_map.contains(model)) {
        return m_models_info_map.at(model).name;
    }
    return {};
}

std::shared_ptr<ModelManager> ModelManager::get_instance(const std::string& modelsInfoJsonPath) {
    static std::shared_ptr<ModelManager> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<ModelManager>(modelsInfoJsonPath); });
    return instance;
}

bool ModelManager::download_model(const Model& model) const {
    if (model == Model::Unknown) {
        return true;
    }
    if (!m_models_info_map.contains(model)) {
        return false;
    }
    const ModelInfo& model_info = m_models_info_map.at(model);
    if (file_system::file_exists(model_info.path)) {
        return true;
    }
    return network::download(model_info.url, "./models");
}

bool ModelManager::is_downloaded(const Model& model) const {
    if (model == Model::Unknown) {
        return true;
    }
    if (!m_models_info_map.contains(model)) {
        return false;
    }
    const ModelInfo& model_info = m_models_info_map.at(model);
    return file_system::file_exists(model_info.path);
}

ModelInfo ModelManager::get_model_info(const Model& model) const {
    if (!m_models_info_map.contains(model)) {
        return ModelInfo{model, "", ""};
    }
    return m_models_info_map.at(model);
}

std::string ModelManager::get_model_url(const Model& model) const {
    if (!m_models_info_map.contains(model)) {
        return {};
    }
    return m_models_info_map.at(model).url;
}

std::string ModelManager::get_model_path(const Model& model) const {
    if (!m_models_info_map.contains(model)) {
        return {};
    }
    if (std::string path = m_models_info_map.at(model).path; file_system::file_exists(path)) {
        return path;
    }
    return {};
}

} // namespace domain::ai::model_manager
