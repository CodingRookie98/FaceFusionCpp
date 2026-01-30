module;
#include <string>
#include <memory>
#include <spdlog/spdlog.h>

/**
 * @file logger.ixx
 * @brief Thread-safe logging module based on spdlog
 * @author
 * CodingRookie
 * @date 2026-01-27
 */
export module foundation.infrastructure.logger;
export import :types;

export namespace foundation::infrastructure::logger {

/**
 * @brief Singleton Logger class wrapper around spdlog
 * @details Provides thread-safe logging capabilities with various log levels
 */
class Logger {
public:
    /**
     * @brief Get the singleton instance of Logger
     * @return Shared pointer to the Logger instance
     */
    static std::shared_ptr<Logger> get_instance();

    /**
     * @brief Initialize Logger with config
     * @param config Logging configuration
     * @note Must be called before first use, otherwise default config is used
     */
    static void initialize(const LoggingConfig& config);

    /**
     * @brief Check if Logger is initialized
     * @return true if initialized
     */
    static bool is_initialized() noexcept;

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Log a message with trace level
     * @param msg The message to log
     */
    void trace(const std::string& msg) const;

    /**
     * @brief Log a message with debug level
     * @param msg The message to log
     */
    void debug(const std::string& msg) const;

    /**
     * @brief Log a message with info level
     * @param msg The message to log
     */
    void info(const std::string& msg) const;

    /**
     * @brief Log a message with warn level
     * @param msg The message to log
     */
    void warn(const std::string& msg) const;

    /**
     * @brief Log a message with error level
     * @param msg The message to log
     */
    void error(const std::string& msg) const;

    /**
     * @brief Log a message with critical level
     * @param msg The message to log
     */
    void critical(const std::string& msg) const;

    /**
     * @brief Log a message with specified level (enum)
     * @param level The log level
     * @param message The message to log
     */
    void log(const LogLevel& level, const std::string& message) const;

    /**
     * @brief Static helper to log a message with specified level (string)
     * @param level The log level string (case-insensitive)
     * @param msg The message to log
     */
    static void log(const std::string& level, const std::string& msg);

private:
    Logger();
    void setup_sinks();
    void start_cleanup_task();
    void cleanup_old_logs();

    std::shared_ptr<spdlog::logger> m_logger;
    LogLevel m_level{LogLevel::Info};
    LoggingConfig m_config;
    bool m_initialized{false};
};

} // namespace foundation::infrastructure::logger
