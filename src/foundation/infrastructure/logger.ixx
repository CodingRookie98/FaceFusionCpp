module;
#include <string>
#include <memory>
#include <spdlog/spdlog.h>

export module foundation.infrastructure.logger;

export namespace foundation::infrastructure::logger {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

class Logger {
public:
    static std::shared_ptr<Logger> get_instance();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void trace(const std::string& msg) const;
    void debug(const std::string& msg) const;
    void info(const std::string& msg) const;
    void warn(const std::string& msg) const;
    void error(const std::string& msg) const;
    void critical(const std::string& msg) const;

    void log(const LogLevel& level, const std::string& message) const;
    static void log(const std::string& level, const std::string& msg);

private:
    Logger();
    std::shared_ptr<spdlog::logger> m_logger;
    LogLevel m_level{LogLevel::Info};
};

} // namespace foundation::infrastructure::logger
