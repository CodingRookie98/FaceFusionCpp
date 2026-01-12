
module;
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <memory>

module foundation.infrastructure.thread_pool;

namespace foundation::infrastructure::thread_pool {

class ThreadPool::Impl {
public:
    Impl(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this] {
                    for (;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                                 [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }
                        task();
                    }
                });
    }

    ~Impl() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
    }

    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) return; // Or throw
            tasks.emplace(std::move(task));
        }
        condition.notify_one();
    }

private:
    std::vector<std::jthread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

// Singleton instance
ThreadPool& ThreadPool::instance() {
    static ThreadPool pool;
    return pool;
}

ThreadPool::ThreadPool() : m_impl(std::make_unique<Impl>(std::thread::hardware_concurrency())) {}
ThreadPool::~ThreadPool() = default; // m_impl unique_ptr requires full type here

void ThreadPool::enqueue_raw(std::function<void()> task) {
    m_impl->enqueue(std::move(task));
}

// Define m_impl in class definition in .ixx using PIMPL?
// Wait, in .ixx I didn't define `m_impl` member in `class ThreadPool`.
// I only had `static ThreadPool& instance(); void enqueue(...)`.
// I need to update .ixx to have `m_impl` OR hide it completely if `ThreadPool` is just interface?
// But `ThreadPool` is a class. If it's a singleton, the instance is static.
// The instance must be stored somewhere.
// In `instance()`, `static ThreadPool pool;` -> `ThreadPool` constructor is called.
// `ThreadPool` needs to store state.
// I should update .ixx to include `std::unique_ptr<Impl> m_impl;` or similar PIMPL.
// Let me check what I wrote in Step 331.
} // namespace foundation::infrastructure::thread_pool
