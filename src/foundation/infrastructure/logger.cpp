module;
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <memory>
#include <string>
#include <filesystem>
#include <mutex>
#include <vector>
#include <iostream>
#include <algorithm>
#include <regex>
#include <thread>
#include <chrono>

module foundation.infrastructure.logger;
import foundation.infrastructure.console;

namespace foundation::infrastructure::logger {

spdlog::level::level_enum to_spdlog_level(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return spdlog::level::trace;
    case LogLevel::Debug: return spdlog::level::debug;
    case LogLevel::Info: return spdlog::level::info;
    case LogLevel::Warn: return spdlog::level::warn;
    case LogLevel::Error: return spdlog::level::err;
    case LogLevel::Critical: return spdlog::level::critical;
    case LogLevel::Off: return spdlog::level::off;
    default: return spdlog::level::info;
    }
}

std::shared_ptr<Logger> Logger::get_instance() {
    static std::once_flag flag;
    static std::shared_ptr<Logger> instance;
    std::call_once(flag, [&]() { instance = std::shared_ptr<Logger>(new Logger()); });
    return instance;
}

void Logger::initialize(const LoggingConfig& config) {
    static std::once_flag init_flag;
    std::call_once(init_flag, [&config]() {
        auto instance = get_instance();
        if (!instance->m_initialized) {
            instance->m_config = config;
            instance->m_initialized = true;
            // Re-setup sinks with new config
            instance->setup_sinks();
        }
    });
}

bool Logger::is_initialized() noexcept {
    return get_instance()->m_initialized;
}

Logger::Logger() {
    // Default initialization (can be overridden by initialize)
    // We set up a basic console sink initially so we don't crash if used before initialize
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        m_logger = std::make_shared<spdlog::logger>("facefusion", console_sink);
        m_logger->set_level(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log init failed: " << ex.what() << std::endl;
    }
}

void Logger::setup_sinks() {
    try {
        std::vector<spdlog::sink_ptr> sinks;

        // 1. Console Sink (always enabled)
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(to_spdlog_level(m_config.level));
        sinks.push_back(console_sink);

        // 2. File Sink (based on rotation policy)
        if (!std::filesystem::exists(m_config.directory)) {
            std::filesystem::create_directories(m_config.directory);
        }
        std::string log_path = (std::filesystem::path(m_config.directory) / "app.log").string();

        spdlog::sink_ptr file_sink;
        switch (m_config.rotation) {
        case RotationPolicy::Daily:
            // Daily rotation at 00:00
            file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_path, 0, 0, false,
                                                                            m_config.max_files);
            break;

        case RotationPolicy::Hourly:
            // Hourly rotation
            file_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(log_path, false,
                                                                             m_config.max_files);
            break;

        case RotationPolicy::Size:
            // Size-based rotation
            file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                log_path, m_config.max_file_size_bytes, m_config.max_files);
            break;
        }

        if (file_sink) {
            file_sink->set_level(to_spdlog_level(m_config.level));
            sinks.push_back(file_sink);
        }

        // 3. Create Logger
        // Replace the existing logger
        auto new_logger =
            std::make_shared<spdlog::logger>("facefusion", sinks.begin(), sinks.end());
        new_logger->set_level(to_spdlog_level(m_config.level));
        new_logger->flush_on(spdlog::level::warn);

        // Atomically swap or just assign (since we are under lock in initialize, but Logger ctor is
        // separate) However, initialize calls setup_sinks under call_once, but setup_sinks might be
        // called from ctor? No, ctor does basic init. initialize calls setup_sinks. We should be
        // careful about thread safety if logging is happening while re-initializing. But
        // initialize() is expected to be called at startup.
        m_logger = new_logger;

        // 4. Start cleanup task if needed
        if (m_config.max_total_size_bytes > 0) { start_cleanup_task(); }

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger setup sinks failed: " << ex.what() << std::endl;
    }
}

void Logger::start_cleanup_task() {
    // Run cleanup immediately once
    cleanup_old_logs();

    // In a real app we might want a background thread, but for now let's just do it once at startup
    // to avoid managing thread lifecycle complexity in this singleton unless requested.
    // The requirement said "periodically check OR trigger on write".
    // Spdlog rotation handles per-file limits.
    // Total size limit needs external management.
    // Let's spawn a detached thread that runs every 5 minutes.
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(5));
            try {
                cleanup_old_logs();
            } catch (...) {
                // Ignore errors in background thread
            }
        }
    }).detach();
}

void Logger::cleanup_old_logs() {
    namespace fs = std::filesystem;
    if (!fs::exists(m_config.directory)) return;

    try {
        std::vector<std::pair<fs::path, fs::file_time_type>> files;
        uint64_t total_size = 0;

        for (const auto& entry : fs::directory_iterator(m_config.directory)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                files.emplace_back(entry.path(), entry.last_write_time());
                total_size += entry.file_size();
            }
        }

        if (total_size <= m_config.max_total_size_bytes) { return; }

        // Sort by time (oldest first)
        std::sort(files.begin(), files.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        for (const auto& [path, _] : files) {
            if (total_size <= m_config.max_total_size_bytes) break;

            auto file_size = fs::file_size(path);
            fs::remove(path);
            total_size -= file_size;

            // Use internal spdlog logging or just cout to avoid recursion loop if we logged here
            // std::cout << "Deleted old log file: " << path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during log cleanup: " << e.what() << std::endl;
    }
}

void Logger::trace(const std::string& msg) const {
    internal_log(spdlog::level::trace, msg);
}

void Logger::debug(const std::string& msg) const {
    internal_log(spdlog::level::debug, msg);
}

void Logger::info(const std::string& msg) const {
    internal_log(spdlog::level::info, msg);
}

void Logger::warn(const std::string& msg) const {
    internal_log(spdlog::level::warn, msg);
}

void Logger::error(const std::string& msg) const {
    internal_log(spdlog::level::err, msg);
}

void Logger::critical(const std::string& msg) const {
    internal_log(spdlog::level::critical, msg);
}

void Logger::internal_log(spdlog::level::level_enum level, const std::string& msg) const {
    foundation::infrastructure::console::ScopedSuspend suspend;
    if (m_logger) m_logger->log(level, msg);
}

void Logger::log(const std::string& level, const std::string& msg) {
    std::string level_t = level;
    std::ranges::for_each(level_t, [](char& c) { c = static_cast<char>(std::tolower(c)); });

    auto instance = get_instance();
    if (level_t == "trace") instance->trace(msg);
    else if (level_t == "debug") instance->debug(msg);
    else if (level_t == "info") instance->info(msg);
    else if (level_t == "warn") instance->warn(msg);
    else if (level_t == "error") instance->error(msg);
    else if (level_t == "critical") instance->critical(msg);
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

uint64_t parse_size_string(const std::string& size_str) {
    if (size_str.empty()) { throw std::invalid_argument("Empty size string"); }

    std::regex pattern(R"((\d+(?:\.\d+)?)\s*(B|KB|MB|GB|TB)?)", std::regex::icase);
    std::smatch match;

    if (!std::regex_match(size_str, match, pattern)) {
        throw std::invalid_argument("Invalid size format: " + size_str);
    }

    double value = std::stod(match[1].str());
    std::string unit = match[2].str();

    std::transform(unit.begin(), unit.end(), unit.begin(), ::toupper);

    uint64_t multiplier = 1;
    if (unit == "KB") multiplier = 1024ULL;
    else if (unit == "MB") multiplier = 1024ULL * 1024;
    else if (unit == "GB") multiplier = 1024ULL * 1024 * 1024;
    else if (unit == "TB") multiplier = 1024ULL * 1024 * 1024 * 1024;

    return static_cast<uint64_t>(value * multiplier);
}

// ScopedTimer Implementation

ScopedTimer::ScopedTimer(std::string name, LogLevel level) :
    m_name(std::move(name)), m_level(level), m_start(std::chrono::steady_clock::now()) {}

ScopedTimer::~ScopedTimer() {
    try {
        auto end = std::chrono::steady_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count();
        // Use simple string concatenation to avoid fmt dependency issues if not configured
        std::string msg = m_name + " took " + std::to_string(duration) + " ms";

        // Use static log method which gets instance safely
        Logger::get_instance()->log(m_level, msg);
    } catch (...) {
        // Destructors should not throw
    }
}

} // namespace foundation::infrastructure::logger
