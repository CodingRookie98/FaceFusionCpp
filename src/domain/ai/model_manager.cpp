/**
 * @file           : model_manager.cpp
 * @brief          : Model management module implementation
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

void to_json(json& j, const ModelInfo& model_info) {
    j = json{
        {"name", model_info.name},
        {"type", model_info.type},
        {"path", model_info.path},
        {"url", model_info.url},
    };
}

void from_json(const json& j, ModelInfo& model_info) {
    model_info.name = j.value("name", model_info.name);
    model_info.type = j.value("type", model_info.type);
    model_info.path = j.value("path", model_info.path);
    model_info.url = j.value("url", model_info.url);
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
            // Use name as the key for the map
            if (!model_info.name.empty()) {
                m_models_info_map[model_info.name] = model_info;
            }
        }
    }
}

std::shared_ptr<ModelManager> ModelManager::get_instance(const std::string& modelsInfoJsonPath) {
    static std::shared_ptr<ModelManager> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<ModelManager>(modelsInfoJsonPath); });
    return instance;
}

bool ModelManager::download_model(const std::string& model_name) const {
    if (model_name.empty()) {
        return true;
    }
    if (!m_models_info_map.contains(model_name)) {
        logger::Logger::get_instance()->warn("Model not found in configuration: " + model_name);
        return false;
    }
    const ModelInfo& model_info = m_models_info_map.at(model_name);
    if (file_system::file_exists(model_info.path)) {
        return true;
    }
    return network::download(model_info.url, "./models");
}

bool ModelManager::is_downloaded(const std::string& model_name) const {
    if (model_name.empty()) {
        return true;
    }
    if (!m_models_info_map.contains(model_name)) {
        return false;
    }
    const ModelInfo& model_info = m_models_info_map.at(model_name);
    return file_system::file_exists(model_info.path);
}

ModelInfo ModelManager::get_model_info(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) {
        return ModelInfo{};
    }
    return m_models_info_map.at(model_name);
}

std::string ModelManager::get_model_url(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) {
        return {};
    }
    return m_models_info_map.at(model_name).url;
}

std::string ModelManager::get_model_path(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) {
        return {};
    }
    if (std::string path = m_models_info_map.at(model_name).path; file_system::file_exists(path)) {
        return path;
    }
    return {};
}

bool ModelManager::has_model(const std::string& model_name) const {
    return m_models_info_map.contains(model_name);
}

} // namespace domain::ai::model_manager
