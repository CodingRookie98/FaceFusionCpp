module;
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <memory>
#include <string>
#include <filesystem>
#include <mutex>
#include <vector>
#include <iostream>
#include <algorithm>

module foundation.infrastructure.logger;

namespace foundation::infrastructure::logger {

std::shared_ptr<Logger> Logger::get_instance() {
    static std::once_flag flag;
    static std::shared_ptr<Logger> instance;
    std::call_once(flag, [&]() { instance = std::shared_ptr<Logger>(new Logger()); });
    return instance;
}

Logger::Logger() {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        if (!std::filesystem::exists("logs")) { std::filesystem::create_directory("logs"); }
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/app.log", true);
        file_sink->set_level(spdlog::level::trace);

        m_logger = std::make_shared<spdlog::logger>(
            "facefusion", spdlog::sinks_init_list{console_sink, file_sink});
        m_logger->set_level(spdlog::level::trace);
        m_logger->flush_on(spdlog::level::trace);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
}

void Logger::trace(const std::string& msg) const {
    if (m_logger) m_logger->trace(msg);
}

void Logger::debug(const std::string& msg) const {
    if (m_logger) m_logger->debug(msg);
}

void Logger::info(const std::string& msg) const {
    if (m_logger) m_logger->info(msg);
}

void Logger::warn(const std::string& msg) const {
    if (m_logger) m_logger->warn(msg);
}

void Logger::error(const std::string& msg) const {
    if (m_logger) m_logger->error(msg);
}

void Logger::critical(const std::string& msg) const {
    if (m_logger) m_logger->critical(msg);
}

void Logger::log(const std::string& level, const std::string& msg) {
    // level to lower
    std::string level_t = level;
    std::ranges::for_each(level_t, [](char& c) { c = std::tolower(c); });
    if (level_t == "trace") {
        get_instance()->trace(msg);
    } else if (level_t == "debug") {
        get_instance()->debug(msg);
    } else if (level_t == "info") {
        get_instance()->info(msg);
    } else if (level_t == "warn") {
        get_instance()->warn(msg);
    } else if (level_t == "error") {
        get_instance()->error(msg);
    } else if (level_t == "critical") {
        get_instance()->critical(msg);
    }
}

void Logger::log(const LogLevel& level, const std::string& message) const {
    switch (level) {
    case LogLevel::Trace: trace(message); break;
    case LogLevel::Debug: debug(message); break;
    case LogLevel::Info: info(message); break;
    case LogLevel::Warn: warn(message); break;
    case LogLevel::Error: error(message); break;
    case LogLevel::Critical: critical(message); break;
    case LogLevel::Off: break;
    }
}
} // namespace foundation::infrastructure::logger
