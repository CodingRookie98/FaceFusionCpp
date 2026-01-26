module;
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * @file concurrent_queue.ixx
 * @brief Thread-safe queue implementation for general use
 */
export module foundation.infrastructure.concurrent_queue;

namespace foundation::infrastructure {

/**
 * @brief Thread-safe queue with blocking push/pop and shutdown capability
 * @tparam T Type of elements in the queue
 */
export template <typename T> class ConcurrentQueue {
public:
    /**
     * @brief Construct a new Concurrent Queue
     * @param max_size Maximum number of elements in the queue
     */
    explicit ConcurrentQueue(size_t max_size) : m_max_size(max_size) {}

    // 禁止拷贝
    ConcurrentQueue(const ConcurrentQueue&) = delete;
    ConcurrentQueue& operator=(const ConcurrentQueue&) = delete;

    /**
     * @brief Push a value into the queue (blocking)
     * @details Blocks until there is space or the queue is shut down
     * @param value The value to push
     */
    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 等待直到队列不满 或 被关闭
        m_not_full.wait(lock, [this] { return m_queue.size() < m_max_size || m_shutdown; });

        if (m_shutdown) return; // 如果已关闭，放弃 push

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
        // 等待直到队列不空 或 被关闭
        m_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        if (m_queue.empty() && m_shutdown) { return std::nullopt; }

        if (m_queue.empty()) return std::nullopt; // double check

        T val = std::move(m_queue.front());
        m_queue.pop();
        m_not_full.notify_one();
        return val;
    }

    /**
     * @brief Try to pop a value without blocking
     * @return std::optional<T> The value, or std::nullopt if empty
     */
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) return std::nullopt;

        T val = std::move(m_queue.front());
        m_queue.pop();
        m_not_full.notify_one();
        return val;
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
        // 唤醒所有
        m_not_empty.notify_all();
        m_not_full.notify_all();
    }

    /**
     * @brief Clear the queue
     */
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> empty;
        std::swap(m_queue, empty);
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

    /**
     * @brief Reset the queue to initial state (not shutdown)
     */
    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = false;
        std::queue<T> empty;
        std::swap(m_queue, empty);
    }

private:
    size_t m_max_size;
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_not_empty;
    std::condition_variable m_not_full;
    bool m_shutdown = false;
};
} // namespace foundation::infrastructure
