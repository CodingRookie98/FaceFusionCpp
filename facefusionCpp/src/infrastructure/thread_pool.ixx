/**
 * @file thread_pool.ixx
 * @brief Thread pool module for concurrent task execution
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides a thread pool implementation using dp::thread_pool library
 */

module;
#include <thread_pool/thread_pool.h>

export module thread_pool;

namespace ffc::infra {

/**
 * @brief Thread pool for concurrent task execution
 * @note This class provides a thread pool implementation using dp::thread_pool library for efficient concurrent task management
 */
export class ThreadPool {
private:
    std::unique_ptr<dp::thread_pool<>> m_thread_pool;

public:
    /**
     * @brief Construct a thread pool with specified number of threads
     * @param thread_num Number of worker threads to create (default: hardware concurrency)
     * @note If thread_num is 0, uses std::thread::hardware_concurrency()
     */
    explicit ThreadPool(const unsigned int& thread_num = std::thread::hardware_concurrency());

    /**
     * @brief Destroy the thread pool and wait for all tasks to complete
     */
    ~ThreadPool();

    /**
     * @brief Get the singleton instance of the thread pool
     * @return std::shared_ptr<ThreadPool> Shared pointer to the singleton instance
     * @note This method implements the singleton pattern for global thread pool access
     */
    static std::shared_ptr<ThreadPool> instance();

    /**
     * @brief Enqueue a task and return a future for the result
     * @tparam Function Function type to execute
     * @tparam Args Argument types for the function
     * @tparam ReturnType Return type of the function
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future<ReturnType> Future object that can be used to retrieve the result
     * @note This method is thread-safe and can be called from multiple threads
     */
    template <typename Function, typename... Args,
              typename ReturnType = std::invoke_result_t<Function&&, Args&&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> enqueue(Function f, Args... args) {
        return m_thread_pool->enqueue(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    /**
     * @brief Enqueue a task without waiting for its completion
     * @tparam Function Function type to execute
     * @tparam Args Argument types for the function
     * @param func Function to execute
     * @param args Arguments to pass to the function
     * @note The task will be executed asynchronously and the function returns immediately
     */
    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...>
    void enqueue_detach(Function&& func, Args&&... args) {
        m_thread_pool->enqueue_detach(std::forward<Function>(func), std::forward<Args>(args)...);
    }

    /**
     * @brief Wait for all pending tasks to complete
     * @note This method blocks until all tasks in the queue have been executed
     */
    void wait_for_tasks();

    /**
     * @brief Get the current number of threads in the pool
     * @return auto Number of threads in the pool
     */
    [[nodiscard]] auto size() const;

    /**
     * @brief Clear all pending tasks from the queue
     * @return size_t Number of tasks that were cleared
     * @note This method does not affect tasks that are currently being executed
     */
    size_t clear_tasks();

    /**
     * @brief Reset the thread pool with a new number of threads
     * @param thread_num New number of worker threads
     * @note This method waits for all current tasks to complete before resetting
     */
    void reset(const unsigned int& thread_num);
};
} // namespace ffc::infra