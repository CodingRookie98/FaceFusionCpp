module;
#include <functional>
#include <memory>

export module foundation.infrastructure.thread_pool;

// import <functional>;
// import <memory>;

export namespace foundation::infrastructure::thread_pool {
class ThreadPool {
public:
    static ThreadPool& instance();

    // Delete copy/move
    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void enqueue(std::function<void()> task);

private:
    ThreadPool();
    ~ThreadPool();

    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace foundation::infrastructure::thread_pool
