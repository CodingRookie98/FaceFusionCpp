/**
 * @file config_types.ixx
 * @brief 配置系统公共类型定义 (枚举、常量、错误处理)
 *
 * 定义配置系统中使用的所有公共类型，包括：
 * - Result<T, E> 错误处理类型
 * - 配置版本常量
 * - 内存策略枚举
 * - 下载策略枚举
 * - 执行顺序枚举
 * - 输出冲突策略枚举
 * - 音频处理策略枚举
 * - 人脸选择模式枚举
 */
module;

// Global Module Fragment - 标准库头文件必须在此处 #include
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <utility>

export module config.types;

export namespace config {

// ============================================================================
// 错误处理类型 (Result<T, E>)
// ============================================================================

/**
 * @brief 配置错误信息
 */
struct ConfigError {
    std::string message; ///< 错误消息
    std::string field;   ///< 出错的字段路径 (可选)

    ConfigError() = default;
    ConfigError(std::string msg) : message(std::move(msg)) {}
    ConfigError(std::string msg, std::string fld) :
        message(std::move(msg)), field(std::move(fld)) {}
};

/**
 * @brief 类似 Rust 的 Result<T, E> 类型
 *
 * 用于表示可能失败的操作结果。
 * - 成功时包含类型 T 的值
 * - 失败时包含类型 E 的错误信息
 *
 * @tparam T 成功值类型
 * @tparam E 错误类型，默认为 ConfigError
 */
template <typename T, typename E = ConfigError> class Result {
public:
    /// 构造成功结果
    static Result Ok(T value) {
        Result r;
        r.data_ = std::move(value);
        return r;
    }

    /// 构造失败结果
    static Result Err(E error) {
        Result r;
        r.data_ = std::move(error);
        return r;
    }

    /// 是否成功
    [[nodiscard]] bool is_ok() const noexcept { return std::holds_alternative<T>(data_); }

    /// 是否失败
    [[nodiscard]] bool is_err() const noexcept { return std::holds_alternative<E>(data_); }

    /// 获取成功值 (调用前需确保 is_ok())
    [[nodiscard]] const T& value() const& { return std::get<T>(data_); }

    [[nodiscard]] T&& value() && { return std::get<T>(std::move(data_)); }

    /// 获取错误 (调用前需确保 is_err())
    [[nodiscard]] const E& error() const& { return std::get<E>(data_); }

    [[nodiscard]] E&& error() && { return std::get<E>(std::move(data_)); }

    /// 获取值或默认值
    [[nodiscard]] T value_or(T default_value) const& {
        return is_ok() ? value() : std::move(default_value);
    }

    /// 显式 bool 转换 (true = 成功)
    explicit operator bool() const noexcept { return is_ok(); }

private:
    Result() = default;
    std::variant<T, E> data_;
};

/// Result<void, E> 的特化声明 (void 成功值)
template <typename E> class Result<void, E> {
public:
    static Result Ok() {
        Result r;
        r.has_error_ = false;
        return r;
    }

    static Result Err(E error) {
        Result r;
        r.error_ = std::move(error);
        r.has_error_ = true;
        return r;
    }

    [[nodiscard]] bool is_ok() const noexcept { return !has_error_; }
    [[nodiscard]] bool is_err() const noexcept { return has_error_; }

    [[nodiscard]] const E& error() const& { return error_; }
    [[nodiscard]] E&& error() && { return std::move(error_); }

    explicit operator bool() const noexcept { return is_ok(); }

private:
    Result() = default;
    E error_;
    bool has_error_ = false;
};

// ============================================================================
// 配置常量
// ============================================================================

/// 支持的配置版本
inline constexpr const char* SUPPORTED_CONFIG_VERSION = "1.0";

// ============================================================================
// 枚举类型定义
// ============================================================================

/**
 * @brief 内存策略
 * 控制处理器的内存管理行为
 */
enum class MemoryStrategy {
    Strict,  ///< 严格模式: 处理器仅在执行时创建，用完即销毁。适合低显存环境
    Tolerant ///< 宽容模式: 处理器在启动时预加载并常驻内存。适合高频任务
};

/**
 * @brief 模型下载策略
 */
enum class DownloadStrategy {
    Force, ///< 无论模型是否存在都强制下载
    Skip,  ///< 模型不存在时跳过下载，返回空路径，后续加载报错退出
    Auto   ///< 模型不存在时自动下载 (默认)
};

/**
 * @brief 执行顺序策略
 */
enum class ExecutionOrder {
    Sequential, ///< 顺序模式: Asset 1 [S1->S2] -> Asset 2 [S1->S2] (默认，省空间)
    Batch       ///< 批处理模式: Step 1 [A1, A2] -> Step 2 [A1, A2] (吞吐量优先)
};

/**
 * @brief 输出文件冲突策略
 */
enum class ConflictPolicy {
    Overwrite, ///< 直接覆盖已存在文件
    Rename,    ///< 自动重命名 (e.g., result_1.mp4)
    Error      ///< 抛出错误，拒绝执行 (默认)
};

/**
 * @brief 音频处理策略
 */
enum class AudioPolicy {
    Copy, ///< 保留原视频音轨并合并到输出 (默认)
    Skip  ///< 跳过音频，输出静音视频
};

/**
 * @brief 人脸选择模式
 */
enum class FaceSelectorMode {
    Reference, ///< 仅处理与参考人脸相似的人脸
    One,       ///< 仅处理检测到的第一个人脸
    Many       ///< 处理所有检测到的人脸 (默认)
};

/**
 * @brief 日志级别
 */
enum class LogLevel { Trace, Debug, Info, Warn, Error };

/**
 * @brief 日志轮转策略
 */
enum class LogRotation { Daily, Hourly, Size };

} // namespace config
