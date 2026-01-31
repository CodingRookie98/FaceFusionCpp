/**
 * @file shutdown_handler.ixx
 * @brief Cross-platform graceful shutdown handler
 * @author CodingRookie
 * @date 2026-01-31
 * @see design.md Section 5.6 - Graceful Shutdown
 */
module;

#include <functional>
#include <chrono>
#include <atomic>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <thread>

#ifdef _WIN32
#include <Windows.h> // Need DWORD, BOOL, WINAPI for handler signature
#endif

export module services.pipeline.shutdown;

import foundation.infrastructure.logger;

export namespace services::pipeline {

/**
 * @brief Shutdown state enumeration
 */
enum class ShutdownState {
    Running,   ///< Normal operation
    Requested, ///< Shutdown requested, waiting for cleanup
    TimedOut,  ///< Graceful shutdown timed out
    Completed  ///< Shutdown completed successfully
};

/**
 * @brief Cross-platform graceful shutdown handler
 * @details Implements design.md Section 5.6 shutdown sequence:
 *          1. Stop accepting new input
 *          2. Wait for in-flight tasks to complete
 *          3. Release resources
 *          4. Force terminate on timeout
 *
 * @note Thread-safe. Singleton pattern for global signal handling.
 *
 * Usage:
 * @code
 *   ShutdownHandler::install([&runner]() {
 *       runner->Cancel();
 *       runner->WaitForCompletion();
 *   }, std::chrono::seconds{5});
 *
 *   // ... run pipeline ...
 *
 *   ShutdownHandler::uninstall();
 * @endcode
 */
class ShutdownHandler {
public:
    using ShutdownCallback = std::function<void()>;
    using TimeoutCallback = std::function<void()>;

    /**
     * @brief Install shutdown handler with callbacks
     * @param on_shutdown Called when shutdown signal received
     * @param timeout Maximum time to wait for graceful shutdown
     * @param on_timeout Called when timeout expires (optional)
     * @note Must be called from main thread before starting pipeline
     */
    static void install(ShutdownCallback on_shutdown,
                        std::chrono::seconds timeout = std::chrono::seconds{5},
                        TimeoutCallback on_timeout = nullptr);

    /**
     * @brief Uninstall shutdown handler and restore default signal handling
     * @note Should be called after pipeline completes
     */
    static void uninstall();

    /**
     * @brief Check if shutdown has been requested
     * @return true if shutdown signal was received
     */
    [[nodiscard]] static bool is_shutdown_requested() noexcept;

    /**
     * @brief Get current shutdown state
     */
    [[nodiscard]] static ShutdownState get_state() noexcept;

    /**
     * @brief Request graceful shutdown programmatically
     * @note Can be called from any thread
     */
    static void request_shutdown();

    /**
     * @brief Block until shutdown completes or times out
     * @return true if graceful shutdown succeeded, false if timed out
     */
    [[nodiscard]] static bool wait_for_shutdown();

    /**
     * @brief Mark shutdown as completed (called after cleanup)
     */
    static void mark_completed();

private:
    ShutdownHandler() = default;
    ~ShutdownHandler() = default;

    // Non-copyable, non-movable
    ShutdownHandler(const ShutdownHandler&) = delete;
    ShutdownHandler& operator=(const ShutdownHandler&) = delete;

    // Platform-specific signal handlers
#ifdef _WIN32
    static BOOL WINAPI windows_console_handler(DWORD ctrl_type);
#else
    static void posix_signal_handler(int signal);
#endif

    // Internal state
    static std::atomic<ShutdownState> s_state;
    static std::atomic<bool> s_installed;
    static ShutdownCallback s_on_shutdown;
    static TimeoutCallback s_on_timeout;
    static std::chrono::seconds s_timeout;

    // Thread management
    static std::mutex s_mutex;
    static std::condition_variable s_cv;
    static std::thread s_watchdog_thread;
    static std::atomic<bool> s_signal_received;
};

} // namespace services::pipeline
