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

namespace ffc::infra {
export class ThreadPool {
public:
    explicit ThreadPool(const unsigned int& thread_num = std::thread::hardware_concurrency());
    ~ThreadPool();

    static std::shared_ptr<ThreadPool> instance();

    template <typename Function, typename... Args,
              typename ReturnType = std::invoke_result_t<Function&&, Args&&...>>
        requires std::invocable<Function, Args...>
    [[nodiscard]] std::future<ReturnType> enqueue(Function f, Args... args) {
        return m_thread_pool->enqueue(std::forward<Function>(f), std::forward<Args>(args)...);
    }

    template <typename Function, typename... Args>
        requires std::invocable<Function, Args...>
    void enqueue_detach(Function&& func, Args&&... args) {
        m_thread_pool->enqueue_detach(std::forward<Function>(func), std::forward<Args>(args)...);
    }

    void wait_for_tasks();

    [[nodiscard]] auto size() const;
    size_t clear_tasks();
    void reset(const unsigned int& thread_num);

private:
    std::unique_ptr<dp::thread_pool<>> m_thread_pool;
};
} // namespace ffc::infra