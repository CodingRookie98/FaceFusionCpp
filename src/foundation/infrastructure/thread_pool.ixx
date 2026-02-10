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
#include <tuple>

/**
 * @file thread_pool.ixx
 * @brief Simple thread pool implementation
 * @author CodingRookie
 *
 * @date 2026-01-27
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
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Enqueue a task to be executed by the thread pool
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future holding the result of the task
     */
    template <class TF, class... TArgs>
    auto enqueue(TF&& f, TArgs&&... args) -> std::future<std::invoke_result_t<TF, TArgs...>> { // NOLINT(cppcoreguidelines-missing-std-forward)
        using return_type = std::invoke_result_t<TF, TArgs...>;

        auto promise = std::make_shared<std::promise<return_type>>();
        std::future<return_type> res = promise->get_future();

        auto task = [f = std::forward<TF>(f), args = std::make_tuple(std::forward<TArgs>(args)...),
                     promise]() mutable {
            try {
                if constexpr (std::is_void_v<return_type>) {
                    std::apply(std::move(f), std::move(args));
                    promise->set_value();
                } else {
                    promise->set_value(std::apply(std::move(f), std::move(args)));
                }
            } catch (...) {
                try {
                    promise->set_exception(std::current_exception());
                } catch (...) { // NOLINT(bugprone-empty-catch)
                    // Ignore exception during set_exception, as we can't do anything about it.
                }
            }
        };

        enqueue_raw(std::move(task));
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
