/**
 * @file app_config.ixx
 * @brief Application global configuration structures
 * @author CodingRookie
 * @date 2026-01-27
 * @details Defines structures corresponding to app_config.yaml, including
 *          inference, resource, logging, and model management settings.
 */
module;

#include <string>
#include <vector>

export module config.app;

export import config.types;

export namespace config {

/**
 * @brief Configuration for TensorRT engine caching
 */
struct EngineCacheConfig {
    bool enable = true;                     ///< Enable or disable engine caching
    std::string path = "./.cache/tensorrt"; ///< Path to store cached engines
};

/**
 * @brief Infrastructure configuration for AI inference
 */
struct InferenceConfig {
    int device_id = 0;              ///< GPU device identifier
    EngineCacheConfig engine_cache; ///< TensorRT cache settings
    std::vector<std::string> default_providers = {"tensorrt", "cuda",
                                                  "cpu"}; ///< Execution provider priority
};

/**
 * @brief Resource and performance configuration
 */
struct ResourceConfig {
    MemoryStrategy memory_strategy = MemoryStrategy::Strict; ///< Memory usage strategy
};

/**
 * @brief Logging system configuration
 */
struct LoggingConfig {
    LogLevel level = LogLevel::Info;           ///< Minimum logging level
    std::string directory = "./logs";          ///< Directory for log files
    LogRotation rotation = LogRotation::Daily; ///< Log file rotation policy
};

/**
 * @brief AI model management configuration
 */
struct ModelsConfig {
    std::string path = "./assets/models";                        ///< Base directory for model files
    DownloadStrategy download_strategy = DownloadStrategy::Auto; ///< Policy for model downloading
};

/**
 * @brief Default model names for various tasks
 */
struct DefaultModels {
    std::string face_detector = "yoloface";
    std::string face_recognizer = "arcface_w600k_r50";
    std::string face_swapper = "inswapper_128";
    std::string face_enhancer = "gfpgan_1.4";
    std::string frame_enhancer = "real_esrgan_x4plus";
    std::string expression_restorer_feature = "live_portrait_feature_extractor";
    std::string expression_restorer_motion = "live_portrait_motion_extractor";
    std::string expression_restorer_generator = "live_portrait_generator";
};

/**
 * @brief Root application configuration structure
 */
struct AppConfig {
    std::string config_version;            ///< Config format version (e.g., "1.0")
    InferenceConfig inference;             ///< Inference-specific settings
    ResourceConfig resource;               ///< Resource management settings
    LoggingConfig logging;                 ///< Logging settings
    ModelsConfig models;                   ///< Model management settings
    DefaultModels default_models;          ///< Default model selections
    std::string temp_directory = "./temp"; ///< Directory for temporary files
};

} // namespace config
