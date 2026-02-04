/**
 * @file config_parser.ixx
 * @brief 配置解析器模块接口
 *
 * 提供 YAML 配置文件的加载、解析、校验功能。
 * 解析策略: YAML -> JSON -> Struct
 */
module;

#include <filesystem>
#include <string>

export module config.parser;

export import config.app;
export import config.task;
export import config.types;
export import config.validator;

export namespace config {

// ============================================================================
// 配置加载 API
// ============================================================================

/**
 * @brief 从 YAML 文件加载 AppConfig
 *
 * @param path YAML 配置文件路径
 * @return Result<AppConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<AppConfig> load_app_config(const std::filesystem::path& path);

/**
 * @brief 从 YAML 文件加载 TaskConfig
 *
 * @param path YAML 配置文件路径
 * @return Result<TaskConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<TaskConfig> load_task_config(const std::filesystem::path& path);

/**
 * @brief 从 YAML 字符串解析 AppConfig
 *
 * @param yaml_content YAML 内容字符串
 * @return Result<AppConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<AppConfig> parse_app_config_from_string(const std::string& yaml_content);

/**
 * @brief 从 YAML 字符串解析 TaskConfig
 *
 * @param yaml_content YAML 内容字符串
 * @return Result<TaskConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<TaskConfig> parse_task_config_from_string(const std::string& yaml_content);

// ============================================================================
// 配置校验 API
// ============================================================================

/**
 * @brief 校验 AppConfig 有效性
 *
 * 校验项:
 * - config_version 必须为 "1.0"
 * - 路径字段不能为空
 * - 枚举值有效性
 *
 * @param config 待校验的配置
 * @return Result<void> 成功返回 Ok，失败返回错误
 */
[[nodiscard]] Result<void, ConfigError> validate_app_config(const AppConfig& config);

/**
 * @brief 校验 TaskConfig 有效性
 *
 * 校验项:
 * - config_version 必须为 "1.0"
 * - task_info.id 不能为空
 * - io.output.path 不能为空
 * - pipeline 至少有一个 step
 * - 数值范围检查
 *
 * @param config 待校验的配置
 * @return Result<void> 成功返回 Ok，失败返回错误
 */
[[nodiscard]] Result<void, ConfigError> validate_task_config(const TaskConfig& config);

// ============================================================================
// 枚举字符串转换 API (用于序列化/反序列化)
// ============================================================================

/// MemoryStrategy <-> string
[[nodiscard]] Result<MemoryStrategy> parse_memory_strategy(const std::string& str);
[[nodiscard]] std::string to_string(MemoryStrategy value);

/// DownloadStrategy <-> string
[[nodiscard]] Result<DownloadStrategy> parse_download_strategy(const std::string& str);
[[nodiscard]] std::string to_string(DownloadStrategy value);

/// ExecutionOrder <-> string
[[nodiscard]] Result<ExecutionOrder> parse_execution_order(const std::string& str);
[[nodiscard]] std::string to_string(ExecutionOrder value);

/// ConflictPolicy <-> string
[[nodiscard]] Result<ConflictPolicy> parse_conflict_policy(const std::string& str);
[[nodiscard]] std::string to_string(ConflictPolicy value);

/// AudioPolicy <-> string
[[nodiscard]] Result<AudioPolicy> parse_audio_policy(const std::string& str);
[[nodiscard]] std::string to_string(AudioPolicy value);

/// FaceSelectorMode <-> string
[[nodiscard]] Result<FaceSelectorMode> parse_face_selector_mode(const std::string& str);
[[nodiscard]] std::string to_string(FaceSelectorMode value);

/// LogLevel <-> string
[[nodiscard]] Result<LogLevel> parse_log_level(const std::string& str);
[[nodiscard]] std::string to_string(LogLevel value);

/// LogRotation <-> string
[[nodiscard]] Result<LogRotation> parse_log_rotation(const std::string& str);
[[nodiscard]] std::string to_string(LogRotation value);

} // namespace config
