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

    // Support both file_name (new) and path (legacy)
    if (j.contains("file_name")) {
        model_info.path = j.at("file_name");
    } else {
        model_info.path = j.value("path", model_info.path);
    }

    model_info.url = j.value("url", model_info.url);
}

ModelRepository::ModelRepository() :
    m_json_file_path(""), m_base_path("./assets/models"),
    m_download_strategy(DownloadStrategy::Auto) {}

std::shared_ptr<ModelRepository> ModelRepository::get_instance() {
    static std::shared_ptr<ModelRepository> instance;
    static std::once_flag flag;

    // Helper struct to allow make_shared to access private constructor
    struct PrivateConstructorTag {
        explicit PrivateConstructorTag() = default;
    };
    struct ConstructibleModelRepository : public ModelRepository {
        ConstructibleModelRepository(PrivateConstructorTag) : ModelRepository() {}
    };

    std::call_once(flag, [&]() {
        instance = std::make_shared<ConstructibleModelRepository>(PrivateConstructorTag());
    });
    return instance;
}

void ModelRepository::set_base_path(const std::string& path) {
    m_base_path = path;
}

void ModelRepository::set_download_strategy(DownloadStrategy strategy) {
    m_download_strategy = strategy;
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

    std::string url;
    std::string final_path_str;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_models_info_map.contains(model_name)) {
            logger::Logger::get_instance()->warn("Model not found in configuration: " + model_name);
            return false;
        }
        const ModelInfo& model_info = m_models_info_map.at(model_name);
        url = model_info.url;
        // Calculate path while holding lock
        final_path_str = get_model_path_internal(model_name);
    }

    // Get the target path using our resolution logic
    // path already calculated above
    if (file_system::file_exists(final_path_str)
        && m_download_strategy != DownloadStrategy::Force) {
        return true;
    }

    // Extract directory from model path
    std::filesystem::path model_path(final_path_str);
    std::string output_dir = model_path.parent_path().string();
    if (output_dir.empty()) { output_dir = "."; }

    // Ensure directory exists
    if (!file_system::dir_exists(output_dir)) { file_system::create_directories(output_dir); }

    return network::download(url, output_dir);
}

bool ModelRepository::is_downloaded(const std::string& model_name) const {
    if (model_name.empty()) { return true; }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_models_info_map.contains(model_name)) { return false; }

    std::string final_path = get_model_path_internal(model_name);
    return file_system::file_exists(final_path);
}

ModelInfo ModelRepository::get_model_info(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_models_info_map.contains(model_name)) { return ModelInfo{}; }
    ModelInfo info = m_models_info_map.at(model_name);
    // Update path in the returned info to match the resolved path
    info.path = get_model_path_internal(model_name);
    return info;
}

std::string ModelRepository::get_model_url(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_models_info_map.contains(model_name)) { return {}; }
    return m_models_info_map.at(model_name).url;
}

std::string ModelRepository::get_model_path(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return get_model_path_internal(model_name);
}

std::string ModelRepository::get_model_path_internal(const std::string& model_name) const {
    if (!m_models_info_map.contains(model_name)) { return {}; }
    const auto& raw_path = m_models_info_map.at(model_name).path;

    if (m_base_path.empty()) { return raw_path; }

    // If base path is set, combine it with the filename of the raw path
    // This supports both full paths (legacy) and filenames (future) in JSON
    return (std::filesystem::path(m_base_path) / std::filesystem::path(raw_path).filename())
        .string();
}

std::string ModelRepository::ensure_model(const std::string& model_name) const {
    if (model_name.empty()) { return {}; }
    std::string final_path;
    DownloadStrategy strategy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_models_info_map.contains(model_name)) {
            logger::Logger::get_instance()->warn("Model not found in configuration: " + model_name);
            return {};
        }
        final_path = get_model_path_internal(model_name);
        strategy = m_download_strategy;
    }

    // If already exists and not force download, return path directly
    if (file_system::file_exists(final_path) && strategy != DownloadStrategy::Force) {
        return final_path;
    }

    // Check skip strategy
    if (strategy == DownloadStrategy::Skip) {
        if (file_system::file_exists(final_path)) { return final_path; }
        logger::Logger::get_instance()->warn("Model missing and download strategy is Skip: "
                                             + model_name);
        return {};
    }

    // Try to download
    logger::Logger::get_instance()->info("Downloading model: " + model_name);
    if (download_model(model_name)) {
        // Verify download succeeded
        if (file_system::file_exists(final_path)) { return final_path; }
    }

    logger::Logger::get_instance()->error("Failed to ensure model: " + model_name);
    return {};
}

bool ModelRepository::has_model(const std::string& model_name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_models_info_map.contains(model_name);
}

} // namespace domain::ai::model_repository
