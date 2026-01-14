/**
 * @file           : model_repository.cpp
 * @brief          : Model repository module implementation
 */

module;
#include <fstream>           // NOLINT(misc-include-cleaner)
#include <filesystem>        // NOLINT(misc-include-cleaner)
#include <nlohmann/json.hpp> // NOLINT(misc-include-cleaner)
#include <mutex>

module domain.ai.model_repository;
import foundation.infrastructure.file_system;
import foundation.infrastructure.network;
import foundation.infrastructure.logger;

namespace domain::ai::model_repository {
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

ModelRepository::ModelRepository() : m_json_file_path("") {}

std::shared_ptr<ModelRepository> ModelRepository::get_instance() {
    static std::shared_ptr<ModelRepository> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance.reset(new ModelRepository()); });
    return instance;
}

void ModelRepository::set_model_info_file_path(const std::string& path) {
    m_json_file_path = path;
    m_models_info_map.clear();

    json models_info_json;
    if (std::ifstream file(m_json_file_path); file.is_open()) {
        models_info_json = nlohmann::json::parse(file);
        file.close();
    } else {
        throw std::runtime_error("Failed to open " + m_json_file_path);
    }

    if (auto model_info_json = models_info_json.items().begin().value();
        model_info_json.is_array()) {
        for (const auto& model_info_json_item : model_info_json.items()) {
            ModelInfo model_info;
            from_json(model_info_json_item.value(), model_info);
            // Use name as the key for the map
            if (!model_info.name.empty()) { m_models_info_map[model_info.name] = model_info; }
        }
    }
}

bool ModelRepository::download_model(const std::string& model_name) const {
    if (model_name.empty()) { return true; }
    if (!m_models_info_map.contains(model_name)) {
        logger::Logger::get_instance()->warn("Model not found in configuration: " + model_name);
        return false;
    }
    const ModelInfo& model_info = m_models_info_map.at(model_name);
    if (file_system::file_exists(model_info.path)) { return true; }

    // Extract directory from model path
    std::filesystem::path model_path(model_info.path);
    std::string output_dir = model_path.parent_path().string();
    if (output_dir.empty()) { output_dir = "."; }

    return network::download(model_info.url, output_dir);
}

bool ModelRepository::is_downloaded(const std::string& model_name) const {
    if (model_name.empty()) { return true; }
    if (!m_models_info_map.contains(model_name)) { return false; }
    const ModelInfo& model_info = m_models_info_map.at(model_name);
    return file_system::file_exists(model_info.path);
}

ModelInfo ModelRepository::get_model_info(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) { return ModelInfo{}; }
    return m_models_info_map.at(model_name);
}

std::string ModelRepository::get_model_url(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) { return {}; }
    return m_models_info_map.at(model_name).url;
}

std::string ModelRepository::get_model_path(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) { return {}; }
    return m_models_info_map.at(model_name).path;
}

std::string ModelRepository::ensure_model(const std::string& model_name) const {
    if (model_name.empty()) { return {}; }
    if (!m_models_info_map.contains(model_name)) {
        logger::Logger::get_instance()->warn("Model not found in configuration: " + model_name);
        return {};
    }

    const ModelInfo& model_info = m_models_info_map.at(model_name);

    // If already exists, return path directly
    if (file_system::file_exists(model_info.path)) { return model_info.path; }

    // Try to download
    logger::Logger::get_instance()->info("Downloading model: " + model_name);
    if (download_model(model_name)) {
        // Verify download succeeded
        if (file_system::file_exists(model_info.path)) { return model_info.path; }
    }

    logger::Logger::get_instance()->error("Failed to ensure model: " + model_name);
    return {};
}

bool ModelRepository::has_model(const std::string& model_name) const {
    return m_models_info_map.contains(model_name);
}

} // namespace domain::ai::model_repository
