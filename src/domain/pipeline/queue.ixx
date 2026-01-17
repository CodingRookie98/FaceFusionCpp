module;
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

export module domain.pipeline:queue;

export namespace domain::pipeline {

template <typename T> class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t max_size) : m_max_size(max_size) {}

    // 禁止拷贝
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 阻塞直到有空间或被关闭
    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待直到队列不满 或 被关闭
        m_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_shutdown; });

        if (m_shutdown) return; // 如果已关闭，放弃 push

        m_queue.push(std::move(value));
        m_not_empty.notify_one();
    }

    // 阻塞直到有数据或被关闭
    // 返回 std::nullopt 表示队列已关闭且为空
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待直到队列不空 或 被关闭
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        if (m_queue.empty() && m_shutdown) { return std::nullopt; }

        if (m_queue.empty()) return std::nullopt; // double check

        T val = std::move(m_queue.front());
        m_queue.pop();
        m_not_full.notify_one();
        return val;
    }

    // 关闭队列，不再接受新数据，唤醒所有等待者
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
        }
        // 唤醒所有
        m_not_empty.notify_all();
        m_not_full.notify_all();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    size_t m_max_size;
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
    bool m_shutdown = false;
};
} // namespace domain::pipeline
