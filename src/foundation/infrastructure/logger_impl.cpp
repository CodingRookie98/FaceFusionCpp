
module;
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>
#include <filesystem>

module foundation.infrastructure.logger;

namespace foundation::infrastructure::logger {

struct Logger::Impl {
    std::shared_ptr<spdlog::logger> logger;
};

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : m_impl(std::make_unique<Impl>()) {
    try {
        // Create a multi-sink logger (console and file)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);

        // Ensure log directory exists
        if (!std::filesystem::exists("logs")) {
            std::filesystem::create_directory("logs");
        }
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app.log", true);
        file_sink->set_level(spdlog::level::trace);

        m_impl->logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
        m_impl->logger->set_level(spdlog::level::trace);

        // Register globally if desired, though we encapsulate
        // spdlog::register_logger(m_impl->logger);
    } catch (const spdlog::spdlog_ex& ex) {
        // Fallback or stderr
    }
}

Logger::~Logger() = default;

void Logger::info(const std::string& msg) {
    if (m_impl->logger) m_impl->logger->info(msg);
}

void Logger::warn(const std::string& msg) {
    if (m_impl->logger) m_impl->logger->warn(msg);
}

void Logger::error(const std::string& msg) {
    if (m_impl->logger) m_impl->logger->error(msg);
}

} // namespace foundation::infrastructure::logger
