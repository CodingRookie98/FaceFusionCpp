/**
 ******************************************************************************
 * @file           : logger.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

module;
#include <spdlog/spdlog.h>

export module logger;

namespace ffc::infra {

export class Logger {
public:
    enum class LogLevel {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    static std::shared_ptr<Logger> get_instance();

    void setLogLevel(const LogLevel& level);
    [[nodiscard]] LogLevel getLogLevel() const;
    void log(const LogLevel& level, const std::string& message) const;
    static void log(const std::string& level, const std::string& msg);
    void trace(const std::string& message) const;
    void debug(const std::string& message) const;
    void info(const std::string& message) const;
    void warn(const std::string& message) const;
    void error(const std::string& message) const;
    void critical(const std::string& message) const;

    Logger();
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    std::shared_ptr<spdlog::logger> m_logger;
    LogLevel m_level;
};

} // namespace ffc::infra
