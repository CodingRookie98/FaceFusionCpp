/**
 * @file session_pool.ixx
 * @brief LRU & TTL Cache for InferenceSession
 */

module;
#include <memory>
#include <string>
#include <chrono>
#include <functional>
#include <optional>

export module foundation.ai.session_pool;

import foundation.ai.inference_session;

export namespace foundation::ai::session_pool {

/**
 * @brief SessionPool Configuration
 */
struct PoolConfig {
    size_t max_entries{3};                         // LRU Capacity
    std::chrono::milliseconds idle_timeout{60000}; // TTL Timeout
    bool enable{true};                             // Enable caching
};

/**
 * @brief Session Cache Pool (LRU + TTL)
 * @details Manages lifecycle of InferenceSession instances, supporting:
 *          - LRU Eviction: Removes least recently used session when max_entries is exceeded
 *          - TTL Expiration: Automatically releases sessions idle for longer than idle_timeout
 */
class SessionPool {
public:
    explicit SessionPool(const PoolConfig& config = {});
    ~SessionPool();

    // No copy
    SessionPool(const SessionPool&) = delete;
    SessionPool& operator=(const SessionPool&) = delete;

    /**
     * @brief Update pool configuration
     * @param config New configuration
     */
    void set_config(const PoolConfig& config);

    /**
     * @brief Get or create a session
     * @param key Unique identifier (usually model_path + options hash)
     * @param factory Factory function to create a new session
     * @return Shared pointer to session
     */
    std::shared_ptr<inference_session::InferenceSession> get_or_create(
        const std::string& key,
        std::function<std::shared_ptr<inference_session::InferenceSession>()> factory);

    /**
     * @brief Manually evict a specific session
     * @param key Session identifier
     * @return true if successfully removed
     */
    bool evict(const std::string& key);

    /**
     * @brief Clear all cached sessions
     */
    void clear();

    /**
     * @brief Trigger TTL cleanup
     * @return Number of sessions cleaned up
     */
    size_t cleanup_expired();

    /**
     * @brief Get current cache size
     */
    [[nodiscard]] size_t size() const noexcept;

    /**
     * @brief Get cache hit statistics
     */
    struct Stats {
        size_t hits{0};
        size_t misses{0};
        size_t evictions{0};
        size_t expirations{0};
    };
    [[nodiscard]] Stats get_stats() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace foundation::ai::session_pool
