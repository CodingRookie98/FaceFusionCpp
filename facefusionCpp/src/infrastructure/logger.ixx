/**
 * @file logger.ixx
 * @brief Logger module for application logging
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides logging functionality using spdlog library
 */

module;
#include <spdlog/spdlog.h>

export module logger;

namespace ffc::infra {

/**
 * @brief Logger class for application logging
 * @note This class provides logging functionality using spdlog library with singleton pattern
 */
export class Logger {
public:
    /**
     * @brief Log level enumeration
     */
    enum class LogLevel {
        Trace,   ///< Trace level for detailed debugging information
        Debug,   ///< Debug level for debugging information
        Info,    ///< Info level for general informational messages
        Warn,    ///< Warning level for warning messages
        Error,   ///< Error level for error messages
        Critical ///< Critical level for critical error messages
    };

private:
    std::shared_ptr<spdlog::logger> m_logger; ///< spdlog logger instance
    LogLevel m_level;                         ///< Current log level

public:
    /**
     * @brief Get the singleton instance of the logger
     * @return std::shared_ptr<Logger> Shared pointer to the singleton instance
     * @note This method implements the singleton pattern for global logger access
     */
    static std::shared_ptr<Logger> get_instance();

    /**
     * @brief Set the log level
     * @param level Log level to set
     */
    void set_log_level(const LogLevel& level);

    /**
     * @brief Get the current log level
     * @return LogLevel Current log level
     */
    [[nodiscard]] LogLevel get_log_level() const;

    /**
     * @brief Log a message with specified level
     * @param level Log level for the message
     * @param message Message to log
     */
    void log(const LogLevel& level, const std::string& message) const;

    /**
     * @brief Log a message with specified level (static method)
     * @param level Log level as string ("trace", "debug", "info", "warn", "error", "critical")
     * @param msg Message to log
     * @note This is a static convenience method for logging without getting the instance
     */
    static void log(const std::string& level, const std::string& msg);

    /**
     * @brief Log a trace message
     * @param message Message to log
     */
    void trace(const std::string& message) const;

    /**
     * @brief Log a debug message
     * @param message Message to log
     */
    void debug(const std::string& message) const;

    /**
     * @brief Log an info message
     * @param message Message to log
     */
    void info(const std::string& message) const;

    /**
     * @brief Log a warning message
     * @param message Message to log
     */
    void warn(const std::string& message) const;

    /**
     * @brief Log an error message
     * @param message Message to log
     */
    void error(const std::string& message) const;

    /**
     * @brief Log a critical message
     * @param message Message to log
     */
    void critical(const std::string& message) const;

    /**
     * @brief Construct a logger instance
     * @note This constructor is private to enforce singleton pattern
     */
    Logger();

    /**
     * @brief Destroy the logger instance
     */
    ~Logger() = default;

    /**
     * @brief Copy constructor (deleted)
     */
    Logger(const Logger&) = delete;

    /**
     * @brief Copy assignment operator (deleted)
     */
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Move constructor (deleted)
     */
    Logger(Logger&&) = delete;

    /**
     * @brief Move assignment operator (deleted)
     */
    Logger& operator=(Logger&&) = delete;
};

} // namespace ffc::infra
