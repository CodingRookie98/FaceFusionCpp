/**
 * @file scoped_timer.ixx
 * @brief RAII-style performance instrumentation timer
 * @author CodingRookie
 * @date 2026-01-31
 * @see design.md Section 5.10.2 - Performance Critical Path
 */
module;

#include <chrono>
#include <string>
#include <string_view>
#include <source_location>

export module foundation.infrastructure.scoped_timer;

import foundation.infrastructure.logger;

export namespace foundation::infrastructure {

/**
 * @brief RAII-style timer for automatic performance logging
 * @details Automatically logs operation duration on destruction.
 *          Supports configurable log levels and custom formatting.
 *
 * Usage:
 * @code
 *   void ProcessFrame() {
 *       ScopedTimer timer("FaceSwapper::process");
 *       // ... processing ...
 *   } // Logs: "[DEBUG] [FaceSwapper::process] completed in 32.5ms"
 * @endcode
 */
class ScopedTimer {
public:
    /**
     * @brief Construct timer with operation name
     * @param operation_name Name to identify this operation in logs
     * @param level Log level for output (default: Debug)
     */
    explicit ScopedTimer(std::string_view operation_name,
                         logger::LogLevel level = logger::LogLevel::Debug);

    /**
     * @brief Construct timer with operation name and entry parameters
     * @param operation_name Name to identify this operation
     * @param entry_params Formatted parameter string for entry log
     * @param level Log level for output
     */
    ScopedTimer(std::string_view operation_name, std::string entry_params,
                logger::LogLevel level = logger::LogLevel::Debug);

    /**
     * @brief Destructor logs exit with duration
     */
    ~ScopedTimer();

    // Non-copyable, non-movable (RAII semantics)
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
    ScopedTimer(ScopedTimer&&) = delete;
    ScopedTimer& operator=(ScopedTimer&&) = delete;

    /**
     * @brief Get elapsed time since construction
     */
    [[nodiscard]] std::chrono::milliseconds elapsed() const;

    /**
     * @brief Get elapsed time in seconds (floating point)
     */
    [[nodiscard]] double elapsed_seconds() const;

    /**
     * @brief Manually log a checkpoint without stopping the timer
     * @param checkpoint_name Name of this checkpoint
     */
    void checkpoint(std::string_view checkpoint_name);

    /**
     * @brief Set result status to include in exit log
     */
    void set_result(std::string_view result);

private:
    std::string m_operation;
    std::string m_result;
    std::chrono::steady_clock::time_point m_start;
    std::chrono::steady_clock::time_point m_last_checkpoint;
    logger::LogLevel m_level;
    bool m_has_entry_log;
};

/**
 * @brief Macro for convenient function-level timing
 * @note Uses source_location to automatically get function name
 */
#define SCOPED_TIMER()                                                                             \
    foundation::infrastructure::ScopedTimer _scoped_timer_##__LINE__(                              \
        std::source_location::current().function_name())

#define SCOPED_TIMER_NAMED(name)                                                                   \
    foundation::infrastructure::ScopedTimer _scoped_timer_##__LINE__(name)

} // namespace foundation::infrastructure
