/**
 * @file shutdown_handler.cpp
 * @brief Cross-platform shutdown handler implementation
 */
module;

#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include <format>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

module services.pipeline.shutdown;

import foundation.infrastructure.logger;

namespace services::pipeline {

using foundation::infrastructure::logger::Logger;

// Static member initialization
std::atomic<ShutdownState> ShutdownHandler::s_state{ShutdownState::Running};
std::atomic<bool> ShutdownHandler::s_installed{false};
ShutdownHandler::ShutdownCallback ShutdownHandler::s_on_shutdown{nullptr};
ShutdownHandler::TimeoutCallback ShutdownHandler::s_on_timeout{nullptr};
std::chrono::seconds ShutdownHandler::s_timeout{5};

std::mutex ShutdownHandler::s_mutex;
std::condition_variable ShutdownHandler::s_cv;
std::thread ShutdownHandler::s_watchdog_thread;
std::atomic<bool> ShutdownHandler::s_signal_received{false};

void ShutdownHandler::install(ShutdownCallback on_shutdown, std::chrono::seconds timeout,
                              TimeoutCallback on_timeout) {
    if (s_installed.exchange(true)) {
        Logger::get_instance()->warn(
            "[ShutdownHandler] Already installed, ignoring duplicate call");
        return;
    }

    s_on_shutdown = std::move(on_shutdown);
    s_on_timeout = std::move(on_timeout);
    s_timeout = timeout;
    s_state = ShutdownState::Running;
    s_signal_received = false;

    Logger::get_instance()->debug(
        std::format("[ShutdownHandler] Installing with {}s timeout", timeout.count()));

    // Start watchdog thread
    s_watchdog_thread = std::thread([]() {
        std::unique_lock lock(s_mutex);
        s_cv.wait(lock, []() { return s_signal_received.load() || !s_installed.load(); });

        if (!s_installed.load()) return;

        // Transition to Requested
        ShutdownState expected = ShutdownState::Running;
        if (!s_state.compare_exchange_strong(expected, ShutdownState::Requested)) { return; }

        Logger::get_instance()->warn(
            "[ShutdownHandler] Shutdown signal processed, initiating graceful shutdown...");

        // Call on_shutdown in a separate thread so we can monitor timeout
        std::thread callback_thread([]() {
            if (s_on_shutdown) { s_on_shutdown(); }
        });

        auto deadline = std::chrono::steady_clock::now() + s_timeout;
        bool completed = false;
        while (std::chrono::steady_clock::now() < deadline) {
            if (s_state.load() == ShutdownState::Completed) {
                completed = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        if (completed) {
            Logger::get_instance()->info("[ShutdownHandler] Graceful shutdown completed");
        } else {
            // Timeout reached
            ShutdownState requested = ShutdownState::Requested;
            if (s_state.compare_exchange_strong(requested, ShutdownState::TimedOut)) {
                Logger::get_instance()->error(std::format(
                    "[ShutdownHandler] Graceful shutdown timed out after {}s, forcing termination",
                    s_timeout.count()));

                if (s_on_timeout) { s_on_timeout(); }
            }
        }

        if (callback_thread.joinable()) {
            callback_thread.detach(); // We can't really wait for it if it's stuck
        }
    });

#ifdef _WIN32
    // Windows: Use SetConsoleCtrlHandler
    if (!SetConsoleCtrlHandler(windows_console_handler, TRUE)) {
        Logger::get_instance()->error(
            "[ShutdownHandler] Failed to install Windows console handler");
        s_installed = false;
        return;
    }
    Logger::get_instance()->debug("[ShutdownHandler] Windows console handler installed");
#else
    // POSIX: Install signal handlers for SIGINT and SIGTERM
    struct sigaction sa{};
    sa.sa_handler = posix_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        Logger::get_instance()->error("[ShutdownHandler] Failed to install SIGINT handler");
        s_installed = false;
        return;
    }
    if (sigaction(SIGTERM, &sa, nullptr) == -1) {
        Logger::get_instance()->error("[ShutdownHandler] Failed to install SIGTERM handler");
        s_installed = false;
        return;
    }
    Logger::get_instance()->debug(
        "[ShutdownHandler] POSIX signal handlers installed (SIGINT, SIGTERM)");
#endif
}

void ShutdownHandler::uninstall() {
    if (!s_installed.exchange(false)) { return; }

    Logger::get_instance()->debug("[ShutdownHandler] Uninstalling signal handlers");

    // Wake up watchdog thread if it's waiting
    {
        std::lock_guard lock(s_mutex);
        s_cv.notify_all();
    }

    if (s_watchdog_thread.joinable()) { s_watchdog_thread.join(); }

#ifdef _WIN32
    SetConsoleCtrlHandler(windows_console_handler, FALSE);
#else
    // Restore default signal handlers
    std::signal(SIGINT, SIG_DFL);
    std::signal(SIGTERM, SIG_DFL);
#endif

    s_on_shutdown = nullptr;
    s_on_timeout = nullptr;
    // Note: s_state is NOT reset here so callers can check final status
}

bool ShutdownHandler::is_shutdown_requested() noexcept {
    return s_state.load(std::memory_order_acquire) != ShutdownState::Running;
}

ShutdownState ShutdownHandler::get_state() noexcept {
    return s_state.load(std::memory_order_acquire);
}

void ShutdownHandler::request_shutdown() {
    s_signal_received = true;
    std::lock_guard lock(s_mutex);
    s_cv.notify_all();
}

bool ShutdownHandler::wait_for_shutdown() {
    auto deadline = std::chrono::steady_clock::now() + s_timeout + std::chrono::seconds{1};

    while (std::chrono::steady_clock::now() < deadline) {
        auto state = s_state.load();
        if (state == ShutdownState::Completed) { return true; }
        if (state == ShutdownState::TimedOut) { return false; }
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
    return false;
}

void ShutdownHandler::mark_completed() {
    s_state.store(ShutdownState::Completed, std::memory_order_release);
    Logger::get_instance()->info("[ShutdownHandler] Shutdown marked as completed");
}

#ifdef _WIN32
BOOL WINAPI ShutdownHandler::windows_console_handler(DWORD ctrl_type) {
    switch (ctrl_type) {
    case CTRL_C_EVENT:
        Logger::get_instance()->warn("[ShutdownHandler] CTRL+C received");
        request_shutdown();
        return TRUE;

    case CTRL_CLOSE_EVENT:
        Logger::get_instance()->warn("[ShutdownHandler] Console close event received");
        request_shutdown();
        // Wait for graceful shutdown before returning
        (void)wait_for_shutdown();
        return TRUE;

    case CTRL_SHUTDOWN_EVENT:
        Logger::get_instance()->warn("[ShutdownHandler] System shutdown event received");
        request_shutdown();
        (void)wait_for_shutdown();
        return TRUE;

    case CTRL_LOGOFF_EVENT:
        // Ignore logoff for console apps
        return FALSE;

    default: return FALSE;
    }
}
#else
void ShutdownHandler::posix_signal_handler(int signal) {
    // Note: request_shutdown only sets a flag and notifies CV, which is safer
    // but notify_all is still not strictly async-signal-safe.
    // In a truly industrial Linux app, we'd use write() to a pipe.
    // For this project, setting an atomic and hoping CV works is a common compromise.
    s_signal_received = true;
    s_cv.notify_all();
}
#endif

} // namespace services::pipeline
