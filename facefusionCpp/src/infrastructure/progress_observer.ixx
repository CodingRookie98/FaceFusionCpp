/**
 * @file progress_observer.ixx
 * @brief Progress observer module for monitoring task progress
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides an interface for progress observation and notification
 */

module;
#include <string>

export module progress_observer;

namespace ffc::infra {

/**
 * @brief Interface for progress observation and notification
 * @note This interface defines methods for observing task progress and receiving notifications
 */
export struct IProgressObserver {
    /**
     * @brief Virtual destructor
     */
    virtual ~IProgressObserver() = default;

    /**
     * @brief Called when a task starts
     * @param total_steps Total number of steps in the task
     * @note This method is called at the beginning of task execution
     */
    virtual void on_start(int total_steps) = 0;

    /**
     * @brief Called when progress is made
     * @param step Current step number
     * @param message Optional message describing the current step
     * @note This method is called for each progress update
     */
    virtual void on_progress(int step, const std::string& message) = 0;

    /**
     * @brief Called when an error occurs
     * @param error_msg Error message describing the error
     * @note This method is called when an error is encountered during task execution
     */
    virtual void on_error(const std::string& error_msg) = 0;

    /**
     * @brief Called when a task completes successfully
     * @note This method is called when the task finishes without errors
     */
    virtual void on_complete() = 0;
};

} // namespace ffc::infra
