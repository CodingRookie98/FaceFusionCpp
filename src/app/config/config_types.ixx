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

export module config.types;

export namespace config {

/**
 * @brief Error information for configuration operations
 */
struct ConfigError {
    std::string message; ///< Descriptive error message
    std::string field;   ///< Path to the field that caused the error (optional)

    ConfigError() = default;
    ConfigError(std::string msg) : message(std::move(msg)) {}
    ConfigError(std::string msg, std::string fld) :
        message(std::move(msg)), field(std::move(fld)) {}
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
