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
[[nodiscard]] Result<AppConfig> LoadAppConfig(const std::filesystem::path& path);

/**
 * @brief 从 YAML 文件加载 TaskConfig
 *
 * @param path YAML 配置文件路径
 * @return Result<TaskConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<TaskConfig> LoadTaskConfig(const std::filesystem::path& path);

/**
 * @brief 从 YAML 字符串解析 AppConfig
 *
 * @param yaml_content YAML 内容字符串
 * @return Result<AppConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<AppConfig> ParseAppConfigFromString(const std::string& yaml_content);

/**
 * @brief 从 YAML 字符串解析 TaskConfig
 *
 * @param yaml_content YAML 内容字符串
 * @return Result<TaskConfig> 成功返回配置对象，失败返回错误
 */
[[nodiscard]] Result<TaskConfig> ParseTaskConfigFromString(const std::string& yaml_content);

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
[[nodiscard]] Result<void, ConfigError> ValidateAppConfig(const AppConfig& config);

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
[[nodiscard]] Result<void, ConfigError> ValidateTaskConfig(const TaskConfig& config);

// ============================================================================
// 枚举字符串转换 API (用于序列化/反序列化)
// ============================================================================

/// MemoryStrategy <-> string
[[nodiscard]] Result<MemoryStrategy> ParseMemoryStrategy(const std::string& str);
[[nodiscard]] std::string ToString(MemoryStrategy value);

/// DownloadStrategy <-> string
[[nodiscard]] Result<DownloadStrategy> ParseDownloadStrategy(const std::string& str);
[[nodiscard]] std::string ToString(DownloadStrategy value);

/// ExecutionOrder <-> string
[[nodiscard]] Result<ExecutionOrder> ParseExecutionOrder(const std::string& str);
[[nodiscard]] std::string ToString(ExecutionOrder value);

/// ConflictPolicy <-> string
[[nodiscard]] Result<ConflictPolicy> ParseConflictPolicy(const std::string& str);
[[nodiscard]] std::string ToString(ConflictPolicy value);

/// AudioPolicy <-> string
[[nodiscard]] Result<AudioPolicy> ParseAudioPolicy(const std::string& str);
[[nodiscard]] std::string ToString(AudioPolicy value);

/// FaceSelectorMode <-> string
[[nodiscard]] Result<FaceSelectorMode> ParseFaceSelectorMode(const std::string& str);
[[nodiscard]] std::string ToString(FaceSelectorMode value);

/// LogLevel <-> string
[[nodiscard]] Result<LogLevel> ParseLogLevel(const std::string& str);
[[nodiscard]] std::string ToString(LogLevel value);

/// LogRotation <-> string
[[nodiscard]] Result<LogRotation> ParseLogRotation(const std::string& str);
[[nodiscard]] std::string ToString(LogRotation value);

} // namespace config
