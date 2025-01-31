/**
 ******************************************************************************
 * @file           : logger.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-19
 ******************************************************************************
 */

module;
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/async.h>

module logger;

namespace ffc {
std::shared_ptr<Logger> Logger::getInstance() {
    static std::shared_ptr<Logger> instance;
    static std::once_flag flag;
    std::call_once(flag, [&]() { instance = std::make_shared<Logger>(); });
    return instance;
}

void Logger::setLogLevel(const Logger::LogLevel &level) {
    m_level = level;
    switch (m_level) {
    case LogLevel::Trace:
        m_logger->set_level(spdlog::level::level_enum::trace);
        break;
    case LogLevel::Debug:
        m_logger->set_level(spdlog::level::level_enum::debug);
        break;
    case LogLevel::Info:
        m_logger->set_level(spdlog::level::level_enum::info);
        break;
    case LogLevel::Warn:
        m_logger->set_level(spdlog::level::level_enum::warn);
        break;
    case LogLevel::Error:
        m_logger->set_level(spdlog::level::level_enum::err);
        break;
    case LogLevel::Critical:
        m_logger->set_level(spdlog::level::level_enum::critical);
        break;
    }
}

void Logger::log(const Logger::LogLevel &level, const std::string &message) const {
    switch (level) {
    case LogLevel::Trace:
        trace(message);
        break;
    case LogLevel::Debug:
        debug(message);
        break;
    case LogLevel::Info:
        info(message);
        break;
    case LogLevel::Warn:
        warn(message);
        break;
    case LogLevel::Error:
        error(message);
        break;
    case LogLevel::Critical:
        critical(message);
        break;
    }
}

void Logger::trace(const std::string &message) const {
    m_logger->trace(message);
}

void Logger::debug(const std::string &message) const {
    m_logger->debug(message);
}

void Logger::info(const std::string &message) const {
    m_logger->info(message);
}

void Logger::warn(const std::string &message) const {
    m_logger->warn(message);
}

void Logger::error(const std::string &message) const {
    m_logger->error(message);
}

void Logger::critical(const std::string &message) const {
    m_logger->critical(message);
}

Logger::Logger() {
    spdlog::init_thread_pool(8192, 4);
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_color_mode(spdlog::color_mode::automatic);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    auto dailySink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("logs/faceFusionCpp.log", 23, 59);
    dailySink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    std::vector<spdlog::sink_ptr> sinks{consoleSink, dailySink};
    m_logger = std::make_shared<spdlog::async_logger>("faceFusionCpp", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    m_logger->set_level(spdlog::level::level_enum::trace);
    spdlog::register_logger(m_logger);
    m_level = LogLevel::Trace;
}

Logger::LogLevel Logger::getLogLevel() const {
    return m_level;
}

void Logger::log(const std::string &level, const std::string &msg) {
    // level to lower
    std::string level_t = level;
    std::ranges::for_each(level_t, [](char &c) { c = std::tolower(c); });
    if (level_t == "trace") {
        getInstance()->trace(msg);
    } else if (level_t == "debug") {
        getInstance()->debug(msg);
    } else if (level_t == "info") {
        getInstance()->info(msg);
    } else if (level_t == "warn") {
        getInstance()->warn(msg);
    } else if (level_t == "error") {
        getInstance()->error(msg);
    } else if (level_t == "critical") {
        getInstance()->critical(msg);
    }
}
} // namespace ffc