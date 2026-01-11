module;
#include <string>
#include <memory>

export module foundation.infrastructure.logger;

export namespace foundation::infrastructure::logger {
class Logger {
public:
    static std::shared_ptr<Logger> get_instance();

    // Delete copy/move
    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&)                 = delete;
    Logger& operator=(Logger&&)      = delete;

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

private:
    Logger();
    ~Logger();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace foundation::infrastructure::logger
