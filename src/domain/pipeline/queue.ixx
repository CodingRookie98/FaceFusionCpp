/**
 * @file queue.ixx
 * @brief Thread-safe queue implementation for the pipeline
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <vector>
#include <algorithm>

export module domain.pipeline:queue;

namespace domain::pipeline {

/**
 * @brief Thread-safe queue with blocking push/pop and shutdown capability
 * @tparam T Type of elements in the queue
 */
export template <typename T_> class ThreadSafeQueue {
public:
    /**
     * @brief Construct a new Thread Safe Queue
     * @param max_size Maximum number of elements in the queue
     */
    explicit ThreadSafeQueue(size_t max_size) : m_max_size(max_size) {}
    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue(ThreadSafeQueue&&) = delete;
    ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

    /**
     * @brief Push a value into the queue (blocking)
     * @details Blocks until there is space or the queue is shut down
     * @param value The value to push
     */
    void push(T_ value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_shutdown; });

        if (m_shutdown) return;

        m_queue.push(std::move(value));
        m_not_empty.notify_one();
    }

    /**
     * @brief Pop a value from the queue (blocking)
     * @details Blocks until there is data or the queue is shut down
     * @return std::optional<T_> The value, or std::nullopt if queue is shutdown and empty
     */
    std::optional<T_> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        if (m_queue.empty() && m_shutdown) { return std::nullopt; }

        if (m_queue.empty()) return std::nullopt;

        T_ val = std::move(m_queue.front());
        m_queue.pop();
        m_not_full.notify_one();
        return val;
    }

    /**
     * @brief Pop multiple items at once to reduce lock contention
     * @param max_items Maximum number of items to retrieve
     * @return Vector of items (empty if queue was empty or stopped)
     */
    std::vector<T_> pop_batch(size_t max_items) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        std::vector<T_> batch;
        if (m_shutdown && m_queue.empty()) return batch;

        const size_t kCount = std::min(max_items, m_queue.size());
        batch.reserve(kCount);

        for (size_t i = 0; i < kCount; ++i) {
            batch.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }

        if (kCount > 0) {
            m_not_full.notify_all(); // Notify producers potentially multiple times or just all
        }
        return batch;
    }

    /**
     * @brief Check if the queue is active (not shut down)
     */
    bool is_active() const {
        const std::scoped_lock kLock(m_mutex);
        return !m_shutdown;
    }

    /**
     * @brief Shutdown the queue
     * @details Wakes up all waiting threads and stops accepting new elements
     */
    void shutdown() {
        {
            const std::scoped_lock kLock(m_mutex);
            m_shutdown = true;
        }
        m_not_empty.notify_all();
        m_not_full.notify_all();
    }

    /**
     * @brief Check if queue is empty
     * @return True if empty
     */
    bool empty() const {
        const std::scoped_lock kLock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief Get current size of the queue
     * @return Number of elements
     */
    size_t size() const {
        const std::scoped_lock kLock(m_mutex);
        return m_queue.size();
    }

private:
    size_t m_max_size;
    std::queue<T_> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
    bool m_shutdown = false;
};
} // namespace domain::pipeline
