/**
 * @file           : model_repository.ixx
 * @brief          : Model repository module for handling AI model information and operations
 */

module;
#include <string>
#include <unordered_map>
#include <memory>
#include <nlohmann/json.hpp>

export module domain.ai.model_repository;

namespace domain::ai::model_repository {

using json = nlohmann::json;

/**
 * @brief Structure containing model information
 */
export struct ModelInfo {
    std::string name; ///< Model name
    std::string type; ///< Model type (e.g., face_swapper, face_enhancer)
    std::string path; ///< Model file path
    std::string url;  ///< Model download URL
};

/**
 * @brief Strategy for downloading models
 */
export enum class DownloadStrategy {
    Auto,  ///< Download if not exists (default)
    Force, ///< Always download (overwrite)
    Skip   ///< Never download, only use local
};

/**
 * @brief Model repository class for handling AI model operations
 */
export class ModelRepository final {
public:
    virtual ~ModelRepository() = default;

    // Delete copy and move constructors and assignment operators
    ModelRepository(const ModelRepository&) = delete;
    ModelRepository& operator=(const ModelRepository&) = delete;
    ModelRepository(ModelRepository&&) = delete;
    ModelRepository& operator=(ModelRepository&&) = delete;

    /**
     * @brief Get the singleton instance of ModelRepository
     * @return std::shared_ptr<ModelRepository> Shared pointer to the singleton instance
     */
    static std::shared_ptr<ModelRepository> get_instance();

    /**
     * @brief Get complete model information
     * @param model_name Model name string
     * @return ModelInfo Complete model information structure
     * @note If model not found, returns empty/default ModelInfo
     */
    [[nodiscard]] ModelInfo get_model_info(const std::string& model_name) const;

    /**
     * @brief Get model file path from configuration (does not verify file exists)
     * @param model_name Model name string
     * @return std::string Model file path from config, empty if model not in config
     */
    [[nodiscard]] std::string get_model_path(const std::string& model_name) const;

    /**
     * @brief Ensure model is available (download if not exists)
     * @param model_name Model name string
     * @return std::string Model file path if available, empty if failed
     * @note This will download the model if it doesn't exist locally
     */
    [[nodiscard]] std::string ensure_model(const std::string& model_name) const;

    /**
     * @brief Get model download URL
     * @param model_name Model name string
     * @return std::string Model download URL
     */
    [[nodiscard]] std::string get_model_url(const std::string& model_name) const;

    /**
     * @brief Download a specific model
     * @param model_name Model name string
     * @return bool True if download succeeded, false otherwise
     */
    [[nodiscard]] bool download_model(const std::string& model_name) const;

    /**
     * @brief Check if a model is already downloaded
     * @param model_name Model name string
     * @return bool True if model is downloaded, false otherwise
     */
    [[nodiscard]] bool is_downloaded(const std::string& model_name) const;

    /**
     * @brief Get path to models information JSON file
     * @return std::string Path to models information JSON file
     */
    [[nodiscard]] std::string get_model_json_file_path() const { return m_json_file_path; }

    /**
     * @brief Check if a model exists in the configuration
     * @param model_name Model name string
     * @return bool True if exists
     */
    [[nodiscard]] bool has_model(const std::string& model_name) const;

    /**
     * @brief Set the path to the models information JSON file
     * @param path Path to
     * the models information JSON file
     */
    void set_model_info_file_path(const std::string& path = "./assets/models_info.json");

    /**
     * @brief Set the base directory for storing models
     * @param path Directory path

     */
    void set_base_path(const std::string& path);

    /**
     * @brief Set the download strategy
     * @param strategy Download strategy enum
 */
    void set_download_strategy(DownloadStrategy strategy);

private:
    /**
     * @brief Construct a new ModelRepository object
     */
    ModelRepository();
    std::string m_json_file_path; ///< Path to models information JSON file
    std::string m_base_path;      ///< Base directory for models
    DownloadStrategy m_download_strategy{DownloadStrategy::Auto}; ///< Current download strategy
    std::unordered_map<std::string, ModelInfo>
        m_models_info_map; ///< Map of model names to ModelInfo structures
};

/**
 * @brief Serialize ModelInfo to JSON
 * @param j JSON object to write to
 * @param model_info ModelInfo object to serialize
 */
export void to_json(json& j, const ModelInfo& model_info);

/**
 * @brief Deserialize ModelInfo from JSON
 * @param j JSON object to read from
 * @param model_info ModelInfo object to deserialize
 */
export void from_json(const json& j, ModelInfo& model_info);

} // namespace domain::ai::model_repository
