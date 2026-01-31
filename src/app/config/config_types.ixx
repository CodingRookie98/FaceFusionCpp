/**
 * @file config_types.ixx
 * @brief Common types for the configuration system
 * @author CodingRookie
 * @date 2026-01-27
 * @details Defines shared enums, constants, and error handling types (Result<T, E>)
 *          used throughout the application configuration.
 */
module;

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <utility>
#include <format>

export module config.types;

export namespace config {

// ============================================================================
// Error Code System (design.md Section 5.3.1)
// ============================================================================

/**
 * @brief Structured error codes for all system operations
 * @details Error codes are organized by category:
 *          - E1xx: System/Infrastructure errors
 *          - E2xx: Configuration errors
 *          - E3xx: Model/Resource errors
 *          - E4xx: Runtime/Business logic errors
 * @see design.md Section 5.3.1 - Error Codes
 */
enum class ErrorCode : int {
    // ─────────────────────────────────────────────────────────────────────────
    // E100-E199: System Level Errors (Requires restart or manual intervention)
    // ─────────────────────────────────────────────────────────────────────────
    E100_SystemError = 100,    ///< Generic system error
    E101_OutOfMemory = 101,    ///< RAM/VRAM exhausted
    E102_DeviceNotFound = 102, ///< CUDA device not available
    E103_ThreadDeadlock = 103, ///< Worker thread deadlock detected
    E104_GPUContextLost = 104, ///< GPU context unexpectedly lost

    // ─────────────────────────────────────────────────────────────────────────
    // E200-E299: Configuration Errors (Fix config and restart)
    // ─────────────────────────────────────────────────────────────────────────
    E200_ConfigError = 200,           ///< Generic configuration error
    E201_YamlFormatInvalid = 201,     ///< YAML syntax error
    E202_ParameterOutOfRange = 202,   ///< Parameter value outside valid range
    E203_ConfigFileNotFound = 203,    ///< Config file does not exist
    E204_ConfigVersionMismatch = 204, ///< Incompatible config_version
    E205_RequiredFieldMissing = 205,  ///< Mandatory field not specified
    E206_InvalidPath = 206,           ///< Path validation failed

    // ─────────────────────────────────────────────────────────────────────────
    // E300-E399: Model/Resource Errors (Check assets directory)
    // ─────────────────────────────────────────────────────────────────────────
    E300_ModelError = 300,               ///< Generic model error
    E301_ModelLoadFailed = 301,          ///< Failed to load model into memory
    E302_ModelFileMissing = 302,         ///< Model file not found
    E303_ModelChecksumMismatch = 303,    ///< Model file corrupted
    E304_ModelVersionIncompatible = 304, ///< Model version not supported

    // ─────────────────────────────────────────────────────────────────────────
    // E400-E499: Runtime/Business Logic Errors (Skip or Fail based on policy)
    // ─────────────────────────────────────────────────────────────────────────
    E400_RuntimeError = 400,      ///< Generic runtime error
    E401_ImageDecodeFailed = 401, ///< Failed to decode image frame
    E402_VideoOpenFailed = 402,   ///< Failed to open video file
    E403_NoFaceDetected = 403,    ///< No face detected in frame
    E404_FaceNotAligned = 404,    ///< Face alignment failed
    E405_ProcessorFailed = 405,   ///< Processor execution failed
    E406_OutputWriteFailed = 406, ///< Failed to write output file
    E407_TaskCancelled = 407,     ///< Task was cancelled by user
};

/**
 * @brief Get error code category name
 */
[[nodiscard]] constexpr std::string_view error_category(ErrorCode code) noexcept {
    int value = static_cast<int>(code);
    if (value >= 100 && value < 200) return "System";
    if (value >= 200 && value < 300) return "Config";
    if (value >= 300 && value < 400) return "Model";
    if (value >= 400 && value < 500) return "Runtime";
    return "Unknown";
}

/**
 * @brief Get human-readable error description
 */
[[nodiscard]] std::string error_code_description(ErrorCode code);

/**
 * @brief Get recommended action for error code
 */
[[nodiscard]] std::string error_code_action(ErrorCode code);

/**
 * @brief Error information for configuration and runtime operations
 * @details Enhanced with structured error codes per design.md Section 5.3.1
 */
struct ConfigError {
    ErrorCode code = ErrorCode::E200_ConfigError;
    std::string message;   ///< Descriptive error message
    std::string yaml_path; ///< YAML path for localization (e.g., "pipeline[1].params.blend_factor")
    int line = 0;          ///< Line number in config file (if available)
    int column = 0;        ///< Column number in config file (if available)

    ConfigError() = default;

    explicit ConfigError(std::string msg) : message(std::move(msg)) {}

    ConfigError(ErrorCode c, std::string msg) : code(c), message(std::move(msg)) {}

    ConfigError(ErrorCode c, std::string msg, std::string path) :
        code(c), message(std::move(msg)), yaml_path(std::move(path)) {}

    ConfigError(ErrorCode c, std::string msg, std::string path, int ln, int col) :
        code(c), message(std::move(msg)), yaml_path(std::move(path)), line(ln), column(col) {}

    /**
     * @brief Format error with code prefix
     * @return Formatted string like "[E403] No face detected in frame 123"
     */
    [[nodiscard]] std::string formatted() const {
        std::string result = std::format("[E{}] ", static_cast<int>(code));

        if (!yaml_path.empty()) { result += yaml_path + ": "; }

        if (line > 0) {
            result += std::format("Line {}", line);
            if (column > 0) { result += std::format(", Column {}", column); }
            result += " - ";
        }

        result += message;
        return result;
    }

    /**
     * @brief Check if this error is recoverable (E4xx errors are typically recoverable)
     */
    [[nodiscard]] bool is_recoverable() const noexcept {
        int value = static_cast<int>(code);
        return value >= 400 && value < 500;
    }

    /**
     * @brief Check if this error is fatal (E1xx, E2xx, E3xx are typically fatal)
     */
    [[nodiscard]] bool is_fatal() const noexcept { return !is_recoverable(); }
};

/**
 * @brief Result type for operations that can fail, similar to Rust's Result
 * @tparam T Type of the success value
 * @tparam E Type of the error value (defaults to ConfigError)
 */
template <typename T, typename E = ConfigError> class Result {
public:
    /**
     * @brief Create a success result
     */
    static Result Ok(T value) {
        Result r;
        r.data_ = std::move(value);
        return r;
    }

    /**
     * @brief Create a failure result
     */
    static Result Err(E error) {
        Result r;
        r.data_ = std::move(error);
        return r;
    }

    /**
     * @brief Check if the result is successful
     */
    [[nodiscard]] bool is_ok() const noexcept { return std::holds_alternative<T>(data_); }

    /**
     * @brief Check if the result contains an error
     */
    [[nodiscard]] bool is_err() const noexcept { return std::holds_alternative<E>(data_); }

    /**
     * @brief Get the success value (requires is_ok())
     */
    [[nodiscard]] const T& value() const& { return std::get<T>(data_); }

    /**
     * @brief Move out the success value (requires is_ok())
     */
    [[nodiscard]] T&& value() && { return std::get<T>(std::move(data_)); }

    /**
     * @brief Get the error value (requires is_err())
     */
    [[nodiscard]] const E& error() const& { return std::get<E>(data_); }

    /**
     * @brief Move out the error value (requires is_err())
     */
    [[nodiscard]] E&& error() && { return std::get<E>(std::move(data_)); }

    /**
     * @brief Get the value or a default if error occurred
     */
    [[nodiscard]] T value_or(T default_value) const& {
        return is_ok() ? value() : std::move(default_value);
    }

    /**
     * @brief Boolean conversion (true if successful)
     */
    explicit operator bool() const noexcept { return is_ok(); }

private:
    Result() = default;
    std::variant<T, E> data_;
};

/**
 * @brief Specialization of Result for void success type
 */
template <typename E> class Result<void, E> {
public:
    /**
     * @brief Create a success result
     */
    static Result Ok() {
        Result r;
        r.has_error_ = false;
        return r;
    }

    /**
     * @brief Create a failure result
     */
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

/**
 * @brief Currently supported configuration format version
 */
inline constexpr const char* SUPPORTED_CONFIG_VERSION = "1.0";

/**
 * @brief Memory usage strategy for AI processors
 */
enum class MemoryStrategy {
    Strict,  ///< Load model on use, release immediately after (low VRAM usage)
    Tolerant ///< Preload model and keep in memory (higher performance)
};

/**
 * @brief Policy for downloading missing model files
 */
enum class DownloadStrategy {
    Force, ///< Always re-download (overwrite existing)
    Skip,  ///< Never download (fail if missing)
    Auto   ///< Download only if missing (default)
};

/**
 * @brief Order of processing for multiple target files
 */
enum class ExecutionOrder {
    Sequential, ///< Process one asset completely before starting next
    Batch       ///< Process one step for all assets before moving to next step
};

/**
 * @brief Policy for handling output file name collisions
 */
enum class ConflictPolicy {
    Overwrite, ///< Replace existing file
    Rename,    ///< Append numeric suffix (e.g., _1)
    Error      ///< Stop and report error (default)
};

/**
 * @brief Policy for processing audio tracks in video files
 */
enum class AudioPolicy {
    Copy, ///< Copy original audio track to result (default)
    Skip  ///< Produce silent video without audio
};

/**
 * @brief Target face selection strategy
 */
enum class FaceSelectorMode {
    Reference, ///< Match faces against source embedding
    One,       ///< Process only the most prominent face
    Many       ///< Process all detected faces (default)
};

/**
 * @brief Application logging levels
 */
enum class LogLevel { Trace, Debug, Info, Warn, Error };

/**
 * @brief Log file rotation interval
 */
enum class LogRotation { Daily, Hourly, Size };

} // namespace config
