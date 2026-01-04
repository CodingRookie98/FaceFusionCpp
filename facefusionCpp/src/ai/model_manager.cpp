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

module model_manager;
import downloader;
import file_system;
import serialize;
import logger;

namespace ffc::ai::model_manager {
using namespace ffc::infra;

using json = nlohmann::json;

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
    return downloader::download(model_info.url, "./models");
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

} // namespace ffc::ai::model_manager
