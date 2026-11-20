/**
 * @file session_pool.cpp
 * @brief Implementation of SessionPool
 */

module;

#include <functional>
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>
#include <cstdio> // Debug

module foundation.ai.session_pool;

import foundation.ai.inference_session;

namespace foundation::ai::session_pool {

using namespace foundation::ai::inference_session;

struct SessionPool::Impl {
    struct CacheEntry {
        std::string key;
        std::shared_ptr<InferenceSession> session;
        std::chrono::steady_clock::time_point last_access;

        // LRU list pointers
        CacheEntry* prev{nullptr};
        CacheEntry* next{nullptr};
    };

    PoolConfig config;
    std::unordered_map<std::string, std::unique_ptr<CacheEntry>> cache;
    CacheEntry* lru_head{nullptr}; // Most recently used
    CacheEntry* lru_tail{nullptr}; // Least recently used
    mutable std::mutex mutex;
    std::condition_variable cv;
    std::unordered_set<std::string> in_flight; // 正在创建的 key（锁外 factory 防重复）
    Stats stats;
    std::chrono::steady_clock::time_point last_cleanup{};

    size_t cleanup_expired_internal(const std::chrono::steady_clock::time_point& now);

    void move_to_head(CacheEntry* entry) {
        if (lru_head == entry) return;
        remove_entry(entry);
        add_to_head(entry);
    }

    void add_to_head(CacheEntry* entry) {
        entry->prev = nullptr;
        entry->next = lru_head;
        if (lru_head) lru_head->prev = entry;
        lru_head = entry;
        if (!lru_tail) lru_tail = entry;
    }

    void remove_entry(CacheEntry* entry) {
        if (entry->prev) entry->prev->next = entry->next;
        if (entry->next) entry->next->prev = entry->prev;
        if (lru_head == entry) lru_head = entry->next;
        if (lru_tail == entry) lru_tail = entry->prev;
    }

    void evict_lru() {
        if (!lru_tail) return;

        auto key = lru_tail->key;
        // The entry will be destroyed when removed from map, so unlink it first
        remove_entry(lru_tail);

        // Remove from map, which deletes the unique_ptr and the Entry
        cache.erase(key);
        stats.evictions++;
    }
};

SessionPool::SessionPool(const PoolConfig& config) : m_impl(std::make_unique<Impl>()) {
    m_impl->config = config;
}

void SessionPool::set_config(const PoolConfig& config) {
    std::lock_guard lock(m_impl->mutex);
    m_impl->config = config;

    // Immediate capacity check if max_entries is reduced
    if (m_impl->config.max_entries > 0) {
        while (m_impl->cache.size() > m_impl->config.max_entries) { m_impl->evict_lru(); }
    }
}

SessionPool::~SessionPool() = default;

std::shared_ptr<InferenceSession> SessionPool::get_or_create(
    const std::string& key, std::function<std::shared_ptr<InferenceSession>()> factory) {
    {
        std::unique_lock lock(m_impl->mutex);

        if (!m_impl->config.enable) { return factory(); }

        // 惰性 TTL 清理：达到清理间隔时移除过期 session（无线程/调度器）
        const auto now = std::chrono::steady_clock::now();
        const auto interval = m_impl->config.cleanup_interval;
        if (interval.count() <= 0 || now - m_impl->last_cleanup >= interval) {
            m_impl->cleanup_expired_internal(now);
            m_impl->last_cleanup = now;
        }

        // Fast path: cache hit
        if (auto it = m_impl->cache.find(key); it != m_impl->cache.end()) {
            auto* entry = it->second.get();
            entry->last_access = std::chrono::steady_clock::now();
            m_impl->move_to_head(entry);
            m_impl->stats.hits++;
            return entry->session;
        }

        // Same key already being created by another thread -> wait for completion
        while (m_impl->in_flight.contains(key)) { m_impl->cv.wait(lock); }
        // Re-check after wait (creator may have finished)
        if (auto it = m_impl->cache.find(key); it != m_impl->cache.end()) {
            auto* entry = it->second.get();
            entry->last_access = std::chrono::steady_clock::now();
            m_impl->move_to_head(entry);
            m_impl->stats.hits++;
            return entry->session;
        }

        m_impl->stats.misses++;
        m_impl->in_flight.insert(key);
    }

    // Factory runs OUTSIDE the pool lock (TRT engine load can take seconds)
    std::shared_ptr<InferenceSession> session;
    try {
        session = factory();
    } catch (...) {
        std::lock_guard lock(m_impl->mutex);
        m_impl->in_flight.erase(key);
        m_impl->cv.notify_all();
        throw;
    }

    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->in_flight.erase(key);

        if (session) {
            // Check capacity
            if (m_impl->config.max_entries > 0
                && m_impl->cache.size() >= m_impl->config.max_entries) {
                m_impl->evict_lru();
            }
            auto entry = std::make_unique<Impl::CacheEntry>();
            entry->key = key;
            entry->session = session;
            entry->last_access = std::chrono::steady_clock::now();
            auto* entry_ptr = entry.get();
            m_impl->cache[key] = std::move(entry);
            m_impl->add_to_head(entry_ptr);
        }
        m_impl->cv.notify_all();
    }

    return session;
}

bool SessionPool::evict(const std::string& key) {
    std::lock_guard lock(m_impl->mutex);
    if (auto it = m_impl->cache.find(key); it != m_impl->cache.end()) {
        m_impl->remove_entry(it->second.get());
        m_impl->cache.erase(it);
        return true;
    }
    return false;
}

void SessionPool::clear() {
    std::lock_guard lock(m_impl->mutex);
    m_impl->cache.clear();
    m_impl->lru_head = nullptr;
    m_impl->lru_tail = nullptr;
}

size_t SessionPool::cleanup_expired() {
    std::lock_guard lock(m_impl->mutex);
    return m_impl->cleanup_expired_internal(std::chrono::steady_clock::now());
}

size_t SessionPool::Impl::cleanup_expired_internal(
    const std::chrono::steady_clock::time_point& now) {
    if (config.idle_timeout.count() <= 0) return 0;

    std::vector<std::string> expired_keys;

    for (const auto& [key, entry] : cache) {
        auto idle_time = now - entry->last_access;
        if (idle_time > config.idle_timeout) { expired_keys.push_back(key); }
    }

    size_t count = 0;
    for (const auto& key : expired_keys) {
        if (auto it = cache.find(key); it != cache.end()) {
            remove_entry(it->second.get());
            cache.erase(it);
            stats.expirations++;
            count++;
        }
    }

    return count;
}

size_t SessionPool::size() const noexcept {
    std::lock_guard lock(m_impl->mutex);
    return m_impl->cache.size();
}

SessionPool::Stats SessionPool::get_stats() const noexcept {
    std::lock_guard lock(m_impl->mutex);
    return m_impl->stats;
}

} // namespace foundation::ai::session_pool
