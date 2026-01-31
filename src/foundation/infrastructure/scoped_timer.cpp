/**
 * @file scoped_timer.cpp
 * @brief ScopedTimer implementation
 */
module;

#include <chrono>
#include <string>
#include <format>

module foundation.infrastructure.scoped_timer;

import foundation.infrastructure.logger;

namespace foundation::infrastructure {

using logger::Logger;
using logger::LogLevel;

ScopedTimer::ScopedTimer(std::string_view operation_name, LogLevel level) :
    m_operation(operation_name), m_start(std::chrono::steady_clock::now()),
    m_last_checkpoint(m_start), m_level(level), m_has_entry_log(false) {}

ScopedTimer::ScopedTimer(std::string_view operation_name, std::string entry_params,
                         LogLevel level) :
    m_operation(operation_name), m_start(std::chrono::steady_clock::now()),
    m_last_checkpoint(m_start), m_level(level), m_has_entry_log(true) {
    auto logger = Logger::get_instance();
    std::string msg = std::format("[{}] Enter {}", m_operation, entry_params);

    switch (m_level) {
    case LogLevel::Trace: logger->trace(msg); break;
    case LogLevel::Debug: logger->debug(msg); break;
    case LogLevel::Info: logger->info(msg); break;
    case LogLevel::Warn: logger->warn(msg); break;
    case LogLevel::Error: logger->error(msg); break;
    default: logger->debug(msg); break;
    }
}

ScopedTimer::~ScopedTimer() {
    auto duration = elapsed();
    auto logger = Logger::get_instance();

    std::string msg;
    if (m_result.empty()) {
        msg = std::format("[{}] Exit duration={}ms", m_operation, duration.count());
    } else {
        msg = std::format("[{}] Exit result={} duration={}ms", m_operation, m_result,
                          duration.count());
    }

    switch (m_level) {
    case LogLevel::Trace: logger->trace(msg); break;
    case LogLevel::Debug: logger->debug(msg); break;
    case LogLevel::Info: logger->info(msg); break;
    case LogLevel::Warn: logger->warn(msg); break;
    case LogLevel::Error: logger->error(msg); break;
    default: logger->debug(msg); break;
    }
}

std::chrono::milliseconds ScopedTimer::elapsed() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start);
}

double ScopedTimer::elapsed_seconds() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - m_start).count();
}

void ScopedTimer::checkpoint(std::string_view checkpoint_name) {
    auto now = std::chrono::steady_clock::now();
    auto since_last =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_checkpoint);
    auto since_start = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start);

    m_last_checkpoint = now;

    auto logger = Logger::get_instance();
    std::string msg = std::format("[{}] Checkpoint '{}' +{}ms (total: {}ms)", m_operation,
                                  checkpoint_name, since_last.count(), since_start.count());
    logger->debug(msg);
}

void ScopedTimer::set_result(std::string_view result) {
    m_result = result;
}

} // namespace foundation::infrastructure
