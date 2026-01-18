module;
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <type_traits>

/**
 * @file thread_pool.ixx
 * @brief Simple thread pool implementation
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.thread_pool;

export namespace foundation::infrastructure::thread_pool {

/**
 * @brief ThreadPool class for managing concurrent tasks
 * @details Implements a fixed-size thread pool with task queue
 */
class ThreadPool {
public:
    /**
     * @brief Get the singleton instance of ThreadPool
     * @return Reference to the singleton instance
     */
    static ThreadPool& instance();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Enqueue a task to be executed by the thread pool
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future holding the result of the task
     */
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();

        // Wrap the task in a void function and pass to the worker
        enqueue_raw([task]() { (*task)(); });

        return res;
    }

private:
    ThreadPool();
    ~ThreadPool();
    void enqueue_raw(std::function<void()> task);

    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace foundation::infrastructure::thread_pool
