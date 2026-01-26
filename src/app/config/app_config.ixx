/**
 * @file app_config.ixx
 * @brief 应用配置结构体定义 (对应 app_config.yaml)
 *
 * 定义应用级配置结构，包括：
 * - 推理基础设施配置
 * - 资源与性能配置
 * - 日志配置
 * - 模型管理配置
 */
module;

// Global Module Fragment - 标准库头文件必须在此处 #include
#include <string>
#include <vector>

export module config.app;

export import config.types;

export namespace config {

/**
 * @brief 引擎缓存配置
 */
struct EngineCacheConfig {
    bool enable = true;
    std::string path = "./.cache/tensorrt";
};

/**
 * @brief 推理基础设施配置
 */
struct InferenceConfig {
    int device_id = 0; ///< 显卡/计算设备ID (扩展预留: 未来支持 device_ids 多卡)
    EngineCacheConfig engine_cache;
    std::vector<std::string> default_providers = {"tensorrt", "cuda", "cpu"};
};

/**
 * @brief 资源与性能配置
 */
struct ResourceConfig {
    MemoryStrategy memory_strategy = MemoryStrategy::Strict;
};

/**
 * @brief 日志配置
 */
struct LoggingConfig {
    LogLevel level = LogLevel::Info;
    std::string directory = "./logs"; ///< 日志目录 (文件名固定为 app.log)
    LogRotation rotation = LogRotation::Daily;
};

/**
 * @brief 模型管理配置
 */
struct ModelsConfig {
    std::string path = "./assets/models"; ///< 模型基础目录
    DownloadStrategy download_strategy = DownloadStrategy::Auto;
};

/**
 * @brief 默认模型配置
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
 * @brief 应用配置 (对应 app_config.yaml)
 *
 * 静态配置，程序启动时加载，全局生效。

 */
struct AppConfig {
    std::string config_version; ///< 配置版本，必须为 "1.0"
    InferenceConfig inference;
    ResourceConfig resource;
    LoggingConfig logging;
    ModelsConfig models;
    DefaultModels default_models;
    std::string temp_directory = "./temp"; ///< 临时文件目录
};

} // namespace config
