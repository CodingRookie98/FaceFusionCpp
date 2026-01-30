/**
 * @file session_pool.cpp
 * @brief Implementation of SessionPool
 */

module;
#include <memory>
#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>
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
    Stats stats;

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

SessionPool::~SessionPool() = default;

std::shared_ptr<InferenceSession> SessionPool::get_or_create(
    const std::string& key, std::function<std::shared_ptr<InferenceSession>()> factory) {
    std::lock_guard lock(m_impl->mutex);

    if (!m_impl->config.enable) { return factory(); }

    // Check cache
    if (auto it = m_impl->cache.find(key); it != m_impl->cache.end()) {
        auto* entry = it->second.get();
        entry->last_access = std::chrono::steady_clock::now();
        m_impl->move_to_head(entry);
        m_impl->stats.hits++;
        return entry->session;
    }

    m_impl->stats.misses++;

    // Create new
    auto session = factory();
    if (!session) return nullptr;

    // Check capacity
    if (m_impl->config.max_entries > 0 && m_impl->cache.size() >= m_impl->config.max_entries) {
        m_impl->evict_lru();
    }

    // Add to cache
    auto entry = std::make_unique<Impl::CacheEntry>();
    entry->key = key;
    entry->session = session;
    entry->last_access = std::chrono::steady_clock::now();

    auto* entry_ptr = entry.get();
    m_impl->cache[key] = std::move(entry);
    m_impl->add_to_head(entry_ptr);

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
    if (m_impl->config.idle_timeout.count() <= 0) return 0;

    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> expired_keys;

    for (const auto& [key, entry] : m_impl->cache) {
        auto idle_time = now - entry->last_access;
        if (idle_time > m_impl->config.idle_timeout) { expired_keys.push_back(key); }
    }

    size_t count = 0;
    for (const auto& key : expired_keys) {
        if (auto it = m_impl->cache.find(key); it != m_impl->cache.end()) {
            m_impl->remove_entry(it->second.get());
            m_impl->cache.erase(it);
            m_impl->stats.expirations++;
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
