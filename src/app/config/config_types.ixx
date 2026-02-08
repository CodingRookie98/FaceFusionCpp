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
#include <cstdint>

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
enum class ErrorCode : std::uint16_t {
    // ─────────────────────────────────────────────────────────────────────────
    // E100-E199: System Level Errors (Requires restart or manual intervention)
    // ─────────────────────────────────────────────────────────────────────────
    E100SystemError = 100,    ///< Generic system error
    E101OutOfMemory = 101,    ///< RAM/VRAM exhausted
    E102DeviceNotFound = 102, ///< CUDA device not available
    E103ThreadDeadlock = 103, ///< Worker thread deadlock detected
    E104GpuContextLost = 104, ///< GPU context unexpectedly lost

    // ─────────────────────────────────────────────────────────────────────────
    // E200-E299: Configuration Errors (Fix config and restart)
    // ─────────────────────────────────────────────────────────────────────────
    E200ConfigError = 200,           ///< Generic configuration error
    E201YamlFormatInvalid = 201,     ///< YAML syntax error
    E202ParameterOutOfRange = 202,   ///< Parameter value outside valid range
    E203ConfigFileNotFound = 203,    ///< Config file does not exist
    E204ConfigVersionMismatch = 204, ///< Incompatible config_version
    E205RequiredFieldMissing = 205,  ///< Mandatory field not specified
    E206InvalidPath = 206,           ///< Path validation failed

    // ─────────────────────────────────────────────────────────────────────────
    // E300-E399: Model/Resource Errors (Check assets directory)
    // ─────────────────────────────────────────────────────────────────────────
    E300ModelError = 300,               ///< Generic model error
    E301ModelLoadFailed = 301,          ///< Failed to load model into memory
    E302ModelFileMissing = 302,         ///< Model file not found
    E303ModelChecksumMismatch = 303,    ///< Model file corrupted
    E304ModelVersionIncompatible = 304, ///< Model version not supported

    // ─────────────────────────────────────────────────────────────────────────
    // E400-E499: Runtime/Business Logic Errors (Skip or Fail based on policy)
    // ─────────────────────────────────────────────────────────────────────────
    E400RuntimeError = 400,      ///< Generic runtime error
    E401ImageDecodeFailed = 401, ///< Failed to decode image frame
    E402VideoOpenFailed = 402,   ///< Failed to open video file
    E403NoFaceDetected = 403,    ///< No face detected in frame
    E404FaceNotAligned = 404,    ///< Face alignment failed
    E405ProcessorFailed = 405,   ///< Processor execution failed
    E406OutputWriteFailed = 406, ///< Failed to write output file
    E407TaskCancelled = 407,     ///< Task was cancelled by user
};

/**
 * @brief Get error code category name
 */
[[nodiscard]] constexpr std::string_view error_category(ErrorCode code) noexcept {
    const auto kValue = static_cast<std::uint16_t>(code);
    if (kValue >= 100 && kValue < 200) return "System";
    if (kValue >= 200 && kValue < 300) return "Config";
    if (kValue >= 300 && kValue < 400) return "Model";
    if (kValue >= 400 && kValue < 500) return "Runtime";
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
    ErrorCode code = ErrorCode::E200ConfigError;
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
        std::string result = std::format("[E{}] ", static_cast<std::uint16_t>(code));

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
        const auto kValue = static_cast<std::uint16_t>(code);
        return kValue >= 400 && kValue < 500;
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
template <typename TValue, typename TError = ConfigError> class Result {
public:
    /**
     * @brief Create a success result
     */
    static Result ok(TValue value) {
        Result r;
        r.m_data = std::move(value);
        return r;
    }

    /**
     * @brief Create a failure result
     */
    static Result err(TError error) {
        Result r;
        r.m_data = std::move(error);
        return r;
    }

    /**
     * @brief Check if the result is successful
     */
    [[nodiscard]] bool is_ok() const noexcept { return std::holds_alternative<TValue>(m_data); }

    /**
     * @brief Check if the result contains an error
     */
    [[nodiscard]] bool is_err() const noexcept { return std::holds_alternative<TError>(m_data); }

    /**
     * @brief Get the success value (requires is_ok())
     */
    [[nodiscard]] const TValue& value() const& { return std::get<TValue>(m_data); }

    /**
     * @brief Move out the success value (requires is_ok())
     */
    [[nodiscard]] TValue&& value() && { return std::get<TValue>(std::move(m_data)); }

    /**
     * @brief Get the error value (requires is_err())
     */
    [[nodiscard]] const TError& error() const& { return std::get<TError>(m_data); }

    /**
     * @brief Move out the error value (requires is_err())
     */
    [[nodiscard]] TError&& error() && { return std::get<TError>(std::move(m_data)); }

    /**
     * @brief Get the value or a default if error occurred
     */
    [[nodiscard]] TValue value_or(TValue default_value) const& {
        return is_ok() ? value() : std::move(default_value);
    }

    /**
     * @brief Boolean conversion (true if successful)
     */
    explicit operator bool() const noexcept { return is_ok(); }

private:
    Result() = default;
    std::variant<TValue, TError> m_data;
};

/**
 * @brief Specialization of Result for void success type
 */
template <typename TError> class Result<void, TError> {
public:
    /**
     * @brief Create a success result
     */
    static Result ok() {
        Result r;
        r.m_has_error = false;
        return r;
    }

    /**
     * @brief Create a failure result
     */
    static Result err(TError error) {
        Result r;
        r.m_error = std::move(error);
        r.m_has_error = true;
        return r;
    }

    [[nodiscard]] bool is_ok() const noexcept { return !m_has_error; }
    [[nodiscard]] bool is_err() const noexcept { return m_has_error; }

    [[nodiscard]] const TError& error() const& { return m_error; }
    [[nodiscard]] TError&& error() && { return std::move(m_error); }

    explicit operator bool() const noexcept { return is_ok(); }

private:
    Result() = default;
    TError m_error;
    bool m_has_error = false;
};

/**
 * @brief Currently supported configuration format version
 */
inline constexpr const char* kSupportedConfigVersion = "1.0";

/**
 * @brief Memory usage strategy for AI processors
 */
enum class MemoryStrategy : std::uint8_t {
    Strict,  ///< Load model on use, release immediately after (low VRAM usage)
    Tolerant ///< Preload model and keep in memory (higher performance)
};

/**
 * @brief Policy for downloading missing model files
 */
enum class DownloadStrategy : std::uint8_t {
    Force, ///< Always re-download (overwrite existing)
    Skip,  ///< Never download (fail if missing)
    Auto   ///< Download only if missing (default)
};

/**
 * @brief Order of processing for multiple target files
 */
enum class ExecutionOrder : std::uint8_t {
    Sequential, ///< Process one asset completely before starting next
    Batch       ///< Process one step for all assets before moving to next step
};

/**
 * @brief Policy for handling output file name collisions
 */
enum class ConflictPolicy : std::uint8_t {
    Overwrite, ///< Replace existing file
    Rename,    ///< Append numeric suffix (e.g., _1)
    Error      ///< Stop and report error (default)
};

/**
 * @brief Policy for processing audio tracks in video files
 */
enum class AudioPolicy : std::uint8_t {
    Copy, ///< Copy original audio track to result (default)
    Skip  ///< Produce silent video without audio
};

/**
 * @brief Target face selection strategy
 */
enum class FaceSelectorMode : std::uint8_t {
    Reference, ///< Match faces against source embedding
    One,       ///< Process only the most prominent face
    Many       ///< Process all detected faces (default)
};

/**
 * @brief Application logging levels
 */
enum class LogLevel : std::uint8_t { Trace, Debug, Info, Warn, Error };

/**
 * @brief Log file rotation interval
 */
enum class LogRotation : std::uint8_t { Daily, Hourly, Size };

} // namespace config
