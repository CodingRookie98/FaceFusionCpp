/**
 * @file config_parser.cpp
 * @brief 配置解析器实现
 *
 * 实现 YAML -> JSON -> Struct 的解析流程
 */
module;

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <nlohmann/json.hpp>

module config.parser;

import foundation.infrastructure.core_utils;
import foundation.infrastructure.file_system;

namespace config {

using json = nlohmann::json;

// ============================================================================
// 内部辅助函数
// ============================================================================

namespace detail {

/// 读取文件内容
Result<std::string> ReadFileContent(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Result<std::string>::err(ConfigError(ErrorCode::E203ConfigFileNotFound,
                                                    "File not found: " + path.string(), "path"));
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return Result<std::string>::err(ConfigError(
            ErrorCode::E203ConfigFileNotFound, "Failed to open file: " + path.string(), "path"));
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return Result<std::string>::ok(buffer.str());
}

/// 转为小写
std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return str;
}

// Supported extensions (should be lower case)
const std::unordered_set<std::string> kImageExtensions = {".png",  ".jpg",  ".jpeg", ".bmp",
                                                          ".webp", ".tiff", ".tif"};

const std::unordered_set<std::string> kVideoExtensions = {".mp4", ".mov", ".avi", ".mkv", ".webm"};

/// 展开路径（如果是目录则递归扫描）
std::vector<std::string> ExpandPaths(const std::vector<std::string>& input_paths,
                                     const std::unordered_set<std::string>& extensions) {
    std::vector<std::string> expanded_paths;

    for (const auto& path_str : input_paths) {
        if (path_str.empty()) continue;

        try {
            if (foundation::infrastructure::file_system::is_dir(path_str)) {
                // Recursive directory scan
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path_str)) {
                    if (entry.is_regular_file()) {
                        auto ext = ToLower(entry.path().extension().string());
                        if (extensions.contains(ext)) {
                            expanded_paths.push_back(
                                std::filesystem::absolute(entry.path()).string());
                        }
                    }
                }
            } else {
                // Keep original file path (validation happens later)
                expanded_paths.push_back(path_str);
            }
        } catch (const std::exception& e) {
            // Log warning? For now just ignore faulty paths during expansion
            // The validator will catch invalid paths later if they are added
        }
    }

    return expanded_paths;
}

/// JSON 安全获取字符串
std::string GetString(const json& j, const std::string& key, const std::string& default_val = "") {
    if (j.contains(key) && j[key].is_string()) { return j[key].get<std::string>(); }
    return default_val;
}

/// JSON 安全获取整数
int GetInt(const json& j, const std::string& key, int default_val = 0) {
    if (j.contains(key) && j[key].is_number_integer()) { return j[key].get<int>(); }
    return default_val;
}

/// JSON 安全获取浮点数
double GetDouble(const json& j, const std::string& key, double default_val = 0.0) {
    if (j.contains(key) && j[key].is_number()) { return j[key].get<double>(); }
    return default_val;
}

/// JSON 安全获取布尔
bool GetBool(const json& j, const std::string& key, bool default_val = false) {
    if (j.contains(key) && j[key].is_boolean()) { return j[key].get<bool>(); }
    return default_val;
}

/// JSON 安全获取字符串数组
std::vector<std::string> GetStringArray(const json& j, const std::string& key) {
    std::vector<std::string> result;
    if (j.contains(key) && j[key].is_array()) {
        for (const auto& item : j[key]) {
            if (item.is_string()) { result.push_back(item.get<std::string>()); }
        }
    }
    return result;
}

/// JSON 安全获取子对象
json GetObject(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_object()) { return j[key]; }
    return json::object();
}

/// JSON 安全获取可选字符串
std::optional<std::string> GetOptionalString(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_string()) { return j[key].get<std::string>(); }
    return std::nullopt;
}

/// JSON 安全获取可选整数
std::optional<int> GetOptionalInt(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_number_integer()) { return j[key].get<int>(); }
    return std::nullopt;
}

/// JSON 安全获取可选浮点数
std::optional<double> GetOptionalDouble(const json& j, const std::string& key) {
    if (j.contains(key) && j[key].is_number()) { return j[key].get<double>(); }
    return std::nullopt;
}

} // namespace detail

// ============================================================================
// 枚举字符串转换实现
// ============================================================================

Result<MemoryStrategy> parse_memory_strategy(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "strict") return Result<MemoryStrategy>::ok(MemoryStrategy::Strict);
    if (lower == "tolerant") return Result<MemoryStrategy>::ok(MemoryStrategy::Tolerant);
    return Result<MemoryStrategy>::err(ConfigError(
        ErrorCode::E202ParameterOutOfRange, "Invalid memory_strategy: " + str, "memory_strategy"));
}

std::string to_string(MemoryStrategy value) {
    switch (value) {
    case MemoryStrategy::Strict: return "strict";
    case MemoryStrategy::Tolerant: return "tolerant";
    }
    return "strict";
}

Result<DownloadStrategy> parse_download_strategy(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "force") return Result<DownloadStrategy>::ok(DownloadStrategy::Force);
    if (lower == "skip") return Result<DownloadStrategy>::ok(DownloadStrategy::Skip);
    if (lower == "auto") return Result<DownloadStrategy>::ok(DownloadStrategy::Auto);
    return Result<DownloadStrategy>::err(ConfigError(ErrorCode::E202ParameterOutOfRange,
                                                     "Invalid download_strategy: " + str,
                                                     "download_strategy"));
}

std::string to_string(DownloadStrategy value) {
    switch (value) {
    case DownloadStrategy::Force: return "force";
    case DownloadStrategy::Skip: return "skip";
    case DownloadStrategy::Auto: return "auto";
    }
    return "auto";
}

Result<ExecutionOrder> parse_execution_order(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "sequential") return Result<ExecutionOrder>::ok(ExecutionOrder::Sequential);
    if (lower == "batch") return Result<ExecutionOrder>::ok(ExecutionOrder::Batch);
    return Result<ExecutionOrder>::err(ConfigError(
        ErrorCode::E202ParameterOutOfRange, "Invalid execution_order: " + str, "execution_order"));
}

std::string to_string(ExecutionOrder value) {
    switch (value) {
    case ExecutionOrder::Sequential: return "sequential";
    case ExecutionOrder::Batch: return "batch";
    }
    return "sequential";
}

Result<ConflictPolicy> parse_conflict_policy(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "overwrite") return Result<ConflictPolicy>::ok(ConflictPolicy::Overwrite);
    if (lower == "rename") return Result<ConflictPolicy>::ok(ConflictPolicy::Rename);
    if (lower == "error") return Result<ConflictPolicy>::ok(ConflictPolicy::Error);
    return Result<ConflictPolicy>::err(ConfigError(
        ErrorCode::E202ParameterOutOfRange, "Invalid conflict_policy: " + str, "conflict_policy"));
}

std::string to_string(ConflictPolicy value) {
    switch (value) {
    case ConflictPolicy::Overwrite: return "overwrite";
    case ConflictPolicy::Rename: return "rename";
    case ConflictPolicy::Error: return "error";
    }
    return "error";
}

Result<AudioPolicy> parse_audio_policy(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "copy") return Result<AudioPolicy>::ok(AudioPolicy::Copy);
    if (lower == "skip") return Result<AudioPolicy>::ok(AudioPolicy::Skip);
    return Result<AudioPolicy>::err(ConfigError(ErrorCode::E202ParameterOutOfRange,
                                                "Invalid audio_policy: " + str, "audio_policy"));
}

std::string to_string(AudioPolicy value) {
    switch (value) {
    case AudioPolicy::Copy: return "copy";
    case AudioPolicy::Skip: return "skip";
    }
    return "copy";
}

Result<FaceSelectorMode> parse_face_selector_mode(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "reference") return Result<FaceSelectorMode>::ok(FaceSelectorMode::Reference);
    if (lower == "one") return Result<FaceSelectorMode>::ok(FaceSelectorMode::One);
    if (lower == "many") return Result<FaceSelectorMode>::ok(FaceSelectorMode::Many);
    return Result<FaceSelectorMode>::err(ConfigError(ErrorCode::E202ParameterOutOfRange,
                                                     "Invalid face_selector_mode: " + str,
                                                     "face_selector_mode"));
}

std::string to_string(FaceSelectorMode value) {
    switch (value) {
    case FaceSelectorMode::Reference: return "reference";
    case FaceSelectorMode::One: return "one";
    case FaceSelectorMode::Many: return "many";
    }
    return "many";
}

Result<LogLevel> parse_log_level(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "trace") return Result<LogLevel>::ok(LogLevel::Trace);
    if (lower == "debug") return Result<LogLevel>::ok(LogLevel::Debug);
    if (lower == "info") return Result<LogLevel>::ok(LogLevel::Info);
    if (lower == "warn") return Result<LogLevel>::ok(LogLevel::Warn);
    if (lower == "error") return Result<LogLevel>::ok(LogLevel::Error);
    return Result<LogLevel>::err(
        ConfigError(ErrorCode::E202ParameterOutOfRange, "Invalid log_level: " + str, "log_level"));
}

std::string to_string(LogLevel value) {
    switch (value) {
    case LogLevel::Trace: return "trace";
    case LogLevel::Debug: return "debug";
    case LogLevel::Info: return "info";
    case LogLevel::Warn: return "warn";
    case LogLevel::Error: return "error";
    }
    return "info";
}

Result<LogRotation> parse_log_rotation(const std::string& str) {
    auto lower = detail::ToLower(str);
    if (lower == "daily") return Result<LogRotation>::ok(LogRotation::Daily);
    if (lower == "hourly") return Result<LogRotation>::ok(LogRotation::Hourly);
    if (lower == "size") return Result<LogRotation>::ok(LogRotation::Size);
    return Result<LogRotation>::err(ConfigError(ErrorCode::E202ParameterOutOfRange,
                                                "Invalid log_rotation: " + str, "log_rotation"));
}

std::string to_string(LogRotation value) {
    switch (value) {
    case LogRotation::Daily: return "daily";
    case LogRotation::Hourly: return "hourly";
    case LogRotation::Size: return "size";
    }
    return "daily";
}

// ============================================================================
// AppConfig 解析实现
// ============================================================================

namespace {

Result<AppConfig> ParseAppConfigFromJson(const json& j) {
    AppConfig config;

    // config_version
    config.config_version = detail::GetString(j, "config_version", "");

    // inference
    auto inference_j = detail::GetObject(j, "inference");
    config.inference.device_id = detail::GetInt(inference_j, "device_id", 0);

    auto engine_cache_j = detail::GetObject(inference_j, "engine_cache");
    config.inference.engine_cache.enable = detail::GetBool(engine_cache_j, "enable", true);
    config.inference.engine_cache.path =
        detail::GetString(engine_cache_j, "path", "./.cache/tensorrt");
    config.inference.engine_cache.max_entries =
        static_cast<size_t>(detail::GetInt(engine_cache_j, "max_entries", 3));
    config.inference.engine_cache.idle_timeout_seconds =
        detail::GetInt(engine_cache_j, "idle_timeout_seconds", 60);

    config.inference.default_providers = detail::GetStringArray(inference_j, "default_providers");
    if (config.inference.default_providers.empty()) {
        config.inference.default_providers = {"tensorrt", "cuda", "cpu"};
    }

    // resource
    auto resource_j = detail::GetObject(j, "resource");
    auto memory_strategy_str = detail::GetString(resource_j, "memory_strategy", "strict");
    auto memory_strategy_r = parse_memory_strategy(memory_strategy_str);
    if (!memory_strategy_r) { return Result<AppConfig>::err(memory_strategy_r.error()); }
    config.resource.memory_strategy = memory_strategy_r.value();

    // logging
    auto logging_j = detail::GetObject(j, "logging");
    auto log_level_str = detail::GetString(logging_j, "level", "info");
    auto log_level_r = parse_log_level(log_level_str);
    if (!log_level_r) { return Result<AppConfig>::err(log_level_r.error()); }
    config.logging.level = log_level_r.value();

    config.logging.directory = detail::GetString(logging_j, "directory", "./logs");

    auto log_rotation_str = detail::GetString(logging_j, "rotation", "daily");
    auto log_rotation_r = parse_log_rotation(log_rotation_str);
    if (!log_rotation_r) { return Result<AppConfig>::err(log_rotation_r.error()); }
    config.logging.rotation = log_rotation_r.value();

    // metrics
    auto metrics_j = detail::GetObject(j, "metrics");
    config.metrics.enable = detail::GetBool(metrics_j, "enable", true);
    config.metrics.step_latency = detail::GetBool(metrics_j, "step_latency", true);
    config.metrics.gpu_memory = detail::GetBool(metrics_j, "gpu_memory", true);
    config.metrics.report_path =
        detail::GetString(metrics_j, "report_path", "./logs/metrics_{timestamp}.json");
    config.metrics.gpu_sample_interval_ms =
        detail::GetInt(metrics_j, "gpu_sample_interval_ms", 1000);

    // models
    auto models_j = detail::GetObject(j, "models");
    config.models.path = detail::GetString(models_j, "path", "./assets/models");

    auto download_strategy_str = detail::GetString(models_j, "download_strategy", "auto");
    auto download_strategy_r = parse_download_strategy(download_strategy_str);
    if (!download_strategy_r) { return Result<AppConfig>::err(download_strategy_r.error()); }
    config.models.download_strategy = download_strategy_r.value();

    // default_models
    auto defaults_j = detail::GetObject(j, "default_models");
    config.default_models.face_detector =
        detail::GetString(defaults_j, "face_detector", "yoloface");
    config.default_models.face_recognizer =
        detail::GetString(defaults_j, "face_recognizer", "arcface_w600k_r50");
    config.default_models.face_swapper =
        detail::GetString(defaults_j, "face_swapper", "inswapper_128");
    config.default_models.face_enhancer =
        detail::GetString(defaults_j, "face_enhancer", "gfpgan_1.4");
    config.default_models.frame_enhancer =
        detail::GetString(defaults_j, "frame_enhancer", "real_esrgan_x4plus");
    config.default_models.expression_restorer_feature = detail::GetString(
        defaults_j, "expression_restorer_feature", "live_portrait_feature_extractor");
    config.default_models.expression_restorer_motion = detail::GetString(
        defaults_j, "expression_restorer_motion", "live_portrait_motion_extractor");
    config.default_models.expression_restorer_generator =
        detail::GetString(defaults_j, "expression_restorer_generator", "live_portrait_generator");

    // default_task_settings
    auto dts_j = detail::GetObject(j, "default_task_settings");

    // IO defaults
    auto io_defaults_j = detail::GetObject(dts_j, "io");
    auto output_defaults_j = detail::GetObject(io_defaults_j, "output");

    config.default_task_settings.io.output.video_encoder =
        detail::GetOptionalString(output_defaults_j, "video_encoder");
    config.default_task_settings.io.output.video_quality =
        detail::GetOptionalInt(output_defaults_j, "video_quality");
    config.default_task_settings.io.output.prefix =
        detail::GetOptionalString(output_defaults_j, "prefix");
    config.default_task_settings.io.output.suffix =
        detail::GetOptionalString(output_defaults_j, "suffix");
    config.default_task_settings.io.output.image_format =
        detail::GetOptionalString(output_defaults_j, "image_format");

    if (auto conflict_str = detail::GetOptionalString(output_defaults_j, "conflict_policy")) {
        auto conflict_r = parse_conflict_policy(*conflict_str);
        if (conflict_r) {
            config.default_task_settings.io.output.conflict_policy = conflict_r.value();
        }
    }

    if (auto audio_str = detail::GetOptionalString(output_defaults_j, "audio_policy")) {
        auto audio_r = parse_audio_policy(*audio_str);
        if (audio_r) { config.default_task_settings.io.output.audio_policy = audio_r.value(); }
    }

    // Resource defaults
    auto resource_defaults_j = detail::GetObject(dts_j, "resource");
    config.default_task_settings.resource.thread_count =
        detail::GetOptionalInt(resource_defaults_j, "thread_count");
    config.default_task_settings.resource.max_queue_size =
        detail::GetOptionalInt(resource_defaults_j, "max_queue_size");

    if (auto exec_order_str = detail::GetOptionalString(resource_defaults_j, "execution_order")) {
        auto exec_order_r = parse_execution_order(*exec_order_str);
        if (exec_order_r) {
            config.default_task_settings.resource.execution_order = exec_order_r.value();
        }
    }

    // Face analysis defaults
    auto fa_defaults_j = detail::GetObject(dts_j, "face_analysis");
    auto detector_defaults_j = detail::GetObject(fa_defaults_j, "face_detector");
    config.default_task_settings.face_analysis.score_threshold =
        detail::GetOptionalDouble(detector_defaults_j, "score_threshold");

    auto recognizer_defaults_j = detail::GetObject(fa_defaults_j, "face_recognizer");
    config.default_task_settings.face_analysis.similarity_threshold =
        detail::GetOptionalDouble(recognizer_defaults_j, "similarity_threshold");

    // temp_directory
    config.temp_directory = detail::GetString(j, "temp_directory", "./temp");

    return Result<AppConfig>::ok(std::move(config));
}

} // anonymous namespace

Result<AppConfig> parse_app_config_from_string(const std::string& yaml_content) {
    try {
        auto j = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_content);
        return ParseAppConfigFromJson(j);
    } catch (const std::exception& e) {
        return Result<AppConfig>::err(ConfigError(ErrorCode::E201YamlFormatInvalid,
                                                  "YAML parse error: " + std::string(e.what())));
    }
}

Result<AppConfig> load_app_config(const std::filesystem::path& path) {
    auto content_r = detail::ReadFileContent(path);
    if (!content_r) { return Result<AppConfig>::err(content_r.error()); }
    return parse_app_config_from_string(content_r.value());
}

// ============================================================================
// TaskConfig 解析实现
// ============================================================================

namespace {

Result<PipelineStep> ParsePipelineStep(const json& step_j) {
    PipelineStep step;
    step.step = detail::GetString(step_j, "step", "");
    step.name = detail::GetString(step_j, "name", "");
    step.enabled = detail::GetBool(step_j, "enabled", true);

    auto params_j = detail::GetObject(step_j, "params");
    auto step_type = detail::ToLower(step.step);

    if (step_type == "face_swapper") {
        FaceSwapperParams params;
        params.model = detail::GetString(params_j, "model", "");

        auto mode_str = detail::GetString(params_j, "face_selector_mode", "many");
        auto mode_r = parse_face_selector_mode(mode_str);
        if (!mode_r) { return Result<PipelineStep>::err(mode_r.error()); }
        params.face_selector_mode = mode_r.value();

        auto ref_path = detail::GetString(params_j, "reference_face_path", "");
        if (!ref_path.empty()) { params.reference_face_path = ref_path; }

        step.params = std::move(params);
    } else if (step_type == "face_enhancer") {
        FaceEnhancerParams params;
        params.model = detail::GetString(params_j, "model", "");
        params.blend_factor = detail::GetDouble(params_j, "blend_factor", 0.8);

        auto mode_str = detail::GetString(params_j, "face_selector_mode", "many");
        auto mode_r = parse_face_selector_mode(mode_str);
        if (!mode_r) { return Result<PipelineStep>::err(mode_r.error()); }
        params.face_selector_mode = mode_r.value();

        auto ref_path = detail::GetString(params_j, "reference_face_path", "");
        if (!ref_path.empty()) { params.reference_face_path = ref_path; }

        step.params = std::move(params);
    } else if (step_type == "expression_restorer") {
        ExpressionRestorerParams params;
        params.model = detail::GetString(params_j, "model", "");
        params.restore_factor = detail::GetDouble(params_j, "restore_factor", 0.8);

        auto mode_str = detail::GetString(params_j, "face_selector_mode", "many");
        auto mode_r = parse_face_selector_mode(mode_str);
        if (!mode_r) { return Result<PipelineStep>::err(mode_r.error()); }
        params.face_selector_mode = mode_r.value();

        auto ref_path = detail::GetString(params_j, "reference_face_path", "");
        if (!ref_path.empty()) { params.reference_face_path = ref_path; }

        step.params = std::move(params);
    } else if (step_type == "frame_enhancer") {
        FrameEnhancerParams params;
        params.model = detail::GetString(params_j, "model", "");
        params.enhance_factor = detail::GetDouble(params_j, "enhance_factor", 0.8);
        step.params = std::move(params);
    } else {
        return Result<PipelineStep>::err(ConfigError(ErrorCode::E202ParameterOutOfRange,
                                                     "Unknown pipeline step type: " + step.step,
                                                     "pipeline.step"));
    }

    return Result<PipelineStep>::ok(std::move(step));
}

Result<TaskConfig> ParseTaskConfigFromJson(const json& j) {
    TaskConfig config;

    // config_version
    config.config_version = detail::GetString(j, "config_version", "");

    // task_info
    auto task_info_j = detail::GetObject(j, "task_info");
    config.task_info.id = detail::GetString(task_info_j, "id", "");
    config.task_info.description = detail::GetString(task_info_j, "description", "");
    config.task_info.enable_logging = detail::GetBool(task_info_j, "enable_logging", true);
    config.task_info.enable_resume = detail::GetBool(task_info_j, "enable_resume", false);

    // io
    auto io_j = detail::GetObject(j, "io");
    auto raw_source_paths = detail::GetStringArray(io_j, "source_paths");
    config.io.source_paths = detail::ExpandPaths(raw_source_paths, detail::kImageExtensions);

    auto raw_target_paths = detail::GetStringArray(io_j, "target_paths");
    // Target can be image or video
    std::unordered_set<std::string> target_exts = detail::kImageExtensions;
    target_exts.insert(detail::kVideoExtensions.begin(), detail::kVideoExtensions.end());
    config.io.target_paths = detail::ExpandPaths(raw_target_paths, target_exts);

    auto output_j = detail::GetObject(io_j, "output");
    config.io.output.path = detail::GetString(output_j, "path", "");
    config.io.output.prefix = detail::GetString(output_j, "prefix", "");
    config.io.output.suffix = detail::GetString(output_j, "suffix", "");
    config.io.output.image_format = detail::GetString(output_j, "image_format", "");
    config.io.output.video_encoder = detail::GetString(output_j, "video_encoder", "");
    config.io.output.video_quality = detail::GetInt(output_j, "video_quality", 0);

    auto conflict_str = detail::GetString(output_j, "conflict_policy", "");
    if (!conflict_str.empty()) {
        auto conflict_r = parse_conflict_policy(conflict_str);
        if (conflict_r) { config.io.output.conflict_policy = conflict_r.value(); }
    }

    auto audio_str = detail::GetString(output_j, "audio_policy", "");
    if (!audio_str.empty()) {
        auto audio_r = parse_audio_policy(audio_str);
        if (audio_r) { config.io.output.audio_policy = audio_r.value(); }
    }

    // resource
    auto resource_j = detail::GetObject(j, "resource");
    config.resource.thread_count = detail::GetInt(resource_j, "thread_count", 0);
    config.resource.max_queue_size = detail::GetInt(resource_j, "max_queue_size", 0);

    auto exec_order_str = detail::GetString(resource_j, "execution_order", "");
    if (!exec_order_str.empty()) {
        auto exec_order_r = parse_execution_order(exec_order_str);
        if (exec_order_r) { config.resource.execution_order = exec_order_r.value(); }
    }

    config.resource.segment_duration_seconds =
        detail::GetInt(resource_j, "segment_duration_seconds", 0);

    // face_analysis
    auto fa_j = detail::GetObject(j, "face_analysis");

    auto detector_j = detail::GetObject(fa_j, "face_detector");
    config.face_analysis.face_detector.models = detail::GetStringArray(detector_j, "models");
    if (config.face_analysis.face_detector.models.empty()) {
        config.face_analysis.face_detector.models = {"yoloface", "retinaface", "scrfd"};
    }
    config.face_analysis.face_detector.score_threshold =
        detail::GetDouble(detector_j, "score_threshold", 0.0);

    auto landmarker_j = detail::GetObject(fa_j, "face_landmarker");
    config.face_analysis.face_landmarker.model = detail::GetString(landmarker_j, "model", "");

    auto recognizer_j = detail::GetObject(fa_j, "face_recognizer");
    config.face_analysis.face_recognizer.model = detail::GetString(recognizer_j, "model", "");
    config.face_analysis.face_recognizer.similarity_threshold =
        detail::GetDouble(recognizer_j, "similarity_threshold", 0.0);

    auto masker_j = detail::GetObject(fa_j, "face_masker");
    config.face_analysis.face_masker.types = detail::GetStringArray(masker_j, "types");
    if (config.face_analysis.face_masker.types.empty()) {
        config.face_analysis.face_masker.types = {"box", "occlusion", "region"};
    }
    config.face_analysis.face_masker.region = detail::GetStringArray(masker_j, "region");
    if (config.face_analysis.face_masker.region.empty()) {
        config.face_analysis.face_masker.region = {"face", "eyes"};
    }

    // pipeline
    if (j.contains("pipeline") && j["pipeline"].is_array()) {
        for (const auto& step_j : j["pipeline"]) {
            auto step_r = ParsePipelineStep(step_j);
            if (!step_r) { return Result<TaskConfig>::err(step_r.error()); }
            config.pipeline.push_back(std::move(step_r).value());
        }
    }

    return Result<TaskConfig>::ok(std::move(config));
}

} // anonymous namespace

Result<TaskConfig> parse_task_config_from_string(const std::string& yaml_content) {
    try {
        auto j = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_content);
        return ParseTaskConfigFromJson(j);
    } catch (const std::exception& e) {
        return Result<TaskConfig>::err(ConfigError(ErrorCode::E201YamlFormatInvalid,
                                                   "YAML parse error: " + std::string(e.what())));
    }
}

Result<TaskConfig> load_task_config(const std::filesystem::path& path) {
    auto content_r = detail::ReadFileContent(path);
    if (!content_r) { return Result<TaskConfig>::err(content_r.error()); }
    return parse_task_config_from_string(content_r.value());
}

// ============================================================================
// 校验实现
// ============================================================================

Result<void, ConfigError> validate_app_config(const AppConfig& config) {
    ConfigValidator validator;
    return validator.validate_or_error(config);
}

Result<void, ConfigError> validate_task_config(const TaskConfig& config) {
    ConfigValidator validator;
    return validator.validate_or_error(config);
}

} // namespace config
