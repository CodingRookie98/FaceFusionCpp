/**
 ******************************************************************************
 * @file           : thread_pool.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 25-2-24
 ******************************************************************************
 */

module;
#include <thread_pool/thread_pool.h>

export module thread_pool;

namespace ffc {
export class ThreadPool {
public:
    explicit ThreadPool(const unsigned int& thread_num = std::thread::hardware_concurrency());
    ~ThreadPool();

    static std::shared_ptr<ThreadPool> Instance();

    template <typename Function, typename... Args,
              typename ReturnType = std::invoke_result_t<Function&&, Args&&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> Enqueue(Function f, Args... args) {
        return thread_pool_->enqueue(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...>
    void EnqueueDetach(Function&& func, Args&&... args) {
        thread_pool_->enqueue_detach(std::forward<Function>(func), std::forward<Args>(args)...);
    }

    void WaitForTasks();

    [[nodiscard]] auto Size() const;
    size_t ClearTasks();

private:
    std::unique_ptr<dp::thread_pool<>> thread_pool_;
};
} // namespace ffc