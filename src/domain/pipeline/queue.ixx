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
export template <typename T> class ThreadSafeQueue {
public:
    /**
     * @brief Construct a new Thread Safe Queue
     * @param max_size Maximum number of elements in the queue
     */
    explicit ThreadSafeQueue(size_t max_size) : m_max_size(max_size) {}

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    /**
     * @brief Push a value into the queue (blocking)
     * @details Blocks until there is space or the queue is shut down
     * @param value The value to push
     */
    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_shutdown; });

        if (m_shutdown) return;

        m_queue.push(std::move(value));
        m_not_empty.notify_one();
    }

    /**
     * @brief Pop a value from the queue (blocking)
     * @details Blocks until there is data or the queue is shut down
     * @return std::optional<T> The value, or std::nullopt if queue is shutdown and empty
     */
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        if (m_queue.empty() && m_shutdown) { return std::nullopt; }

        if (m_queue.empty()) return std::nullopt;

        T val = std::move(m_queue.front());
        m_queue.pop();
        m_not_full.notify_one();
        return val;
    }

    /**
     * @brief Pop multiple items at once to reduce lock contention
     * @param max_items Maximum number of items to retrieve
     * @return Vector of items (empty if queue was empty or stopped)
     */
    std::vector<T> pop_batch(size_t max_items) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        std::vector<T> batch;
        if (m_shutdown && m_queue.empty()) return batch;

        size_t count = std::min(max_items, m_queue.size());
        batch.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            batch.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }

        if (count > 0) {
            m_not_full.notify_all(); // Notify producers potentially multiple times or just all
        }
        return batch;
    }

    /**
     * @brief Check if the queue is active (not shut down)
     */
    bool is_active() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_shutdown;
    }

    /**
     * @brief Shutdown the queue
     * @details Wakes up all waiting threads and stops accepting new elements
     */
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
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
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    /**
     * @brief Get current size of the queue
     * @return Number of elements
     */
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
