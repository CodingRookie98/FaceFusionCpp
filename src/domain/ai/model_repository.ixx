/**
 * @file model_repository.ixx
 * @brief Repository for managing AI model information and lifecycle
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

export module domain.ai.model_repository;

namespace domain::ai::model_repository {

using json = nlohmann::json;

/**
 * @brief Metadata for a single AI model
 */
export struct ModelInfo {
    std::string name; ///< Unique identifier for the model
    std::string type; ///< Model category (e.g. face_detector)
    std::string path; ///< Local filesystem path to the model file
    std::string url;  ///< Remote download URL
};

/**
 * @brief Strategy for handling missing model files
 */
export enum class DownloadStrategy : std::uint8_t {
    Auto,  ///< Download automatically if not found locally
    Force, ///< Always re-download from remote
    Skip   ///< Never download (fail if missing)
};

/**
 * @brief Central repository for all AI models used by the application
 * @details Handles model configuration loading, existence verification, and
 *          automated downloading using standard network utilities.
 */
export class ModelRepository {
public:
    virtual ~ModelRepository() = default;

    ModelRepository(const ModelRepository&) = delete;
    ModelRepository& operator=(const ModelRepository&) = delete;
    ModelRepository(ModelRepository&&) = delete;
    ModelRepository& operator=(ModelRepository&&) = delete;

    /**
     * @brief Get the singleton instance of the repository
     */
    static std::shared_ptr<ModelRepository> get_instance();

    /**
     * @brief Retrieve metadata for a named model
     */
    [[nodiscard]] ModelInfo get_model_info(const std::string& model_name) const;

    /**
     * @brief Get the configured path for a model (may not exist)
     */
    [[nodiscard]] std::string get_model_path(const std::string& model_name) const;

    /**
     * @brief Ensure a model is available locally (downloads if necessary)
     * @return Path to the verified model file, or empty string on failure
     */
    [[nodiscard]] std::string ensure_model(const std::string& model_name) const;

    /**
     * @brief Get the download URL for a model
     */
    [[nodiscard]] std::string get_model_url(const std::string& model_name) const;

    /**
     * @brief Manually trigger a model download
     */
    [[nodiscard]] bool download_model(const std::string& model_name) const;

    /**
     * @brief Check if a model file exists on disk
     */
    [[nodiscard]] bool is_downloaded(const std::string& model_name) const;

    /**
     * @brief Get the path to the models registry JSON file
     */
    [[nodiscard]] std::string get_model_json_file_path() const { return m_json_file_path; }

    /**
     * @brief Check if a model is defined in the repository
     */
    [[nodiscard]] bool has_model(const std::string& model_name) const;

    /**
     * @brief Set the path to the model info registry file
     */
    void set_model_info_file_path(const std::string& path = "./assets/models_info.json");

    /**
     * @brief Set the root directory for model storage
     */
    void set_base_path(const std::string& path);

    /**
     * @brief Set the current download strategy
     */
    void set_download_strategy(DownloadStrategy strategy);

private:
    ModelRepository();
    std::string m_json_file_path;
    std::string m_base_path;
    DownloadStrategy m_download_strategy{DownloadStrategy::Auto};
    std::unordered_map<std::string, ModelInfo> m_models_info_map;
};

/**
 * @brief Serialize ModelInfo to JSON
 */
export void to_json(json& j, const ModelInfo& model_info);

/**
 * @brief Deserialize ModelInfo from JSON
 */
export void from_json(const json& j, ModelInfo& model_info);

} // namespace domain::ai::model_repository
