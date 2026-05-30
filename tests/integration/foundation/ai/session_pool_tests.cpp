/**
 * @file session_pool_tests.cpp
 * @brief Integration tests for SessionPool
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

import foundation.ai.inference_session;
import foundation.ai.session_pool;

using namespace foundation::ai::inference_session;
using namespace foundation::ai::session_pool;

class SessionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SessionPoolTest, BasicGetOrCreate) {
    PoolConfig config;
    config.max_entries = 5;
    config.enable = true;
    SessionPool pool(config);

    bool factory_called = false;
    auto factory = [&]() {
        factory_called = true;
        return std::make_shared<InferenceSession>();
    };

    auto session1 = pool.get_or_create("key1", factory);
    EXPECT_TRUE(factory_called);
    EXPECT_NE(session1, nullptr);

    factory_called = false;
    auto session2 = pool.get_or_create("key1", factory);
    EXPECT_FALSE(factory_called); // Should hit cache
    EXPECT_EQ(session1, session2);
}

TEST_F(SessionPoolTest, LRUEviction) {
    PoolConfig config;
    config.max_entries = 2; // Small capacity
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<InferenceSession>(); };

    auto s1 = pool.get_or_create("key1", factory);
    auto s2 = pool.get_or_create("key2", factory);

    EXPECT_EQ(pool.size(), 2);

    // Access key1 to make it MRU
    pool.get_or_create("key1", factory);

    // Add key3, should evict key2 (LRU)
    auto s3 = pool.get_or_create("key3", factory);
    EXPECT_EQ(pool.size(), 2);

    auto stats = pool.get_stats();
    EXPECT_EQ(stats.evictions, 1);

    // Verify key1 is still there (should not invoke factory)
    bool new_created = false;
    pool.get_or_create("key1", [&]() {
        new_created = true;
        return std::make_shared<InferenceSession>();
    });
    EXPECT_FALSE(new_created);

    // Verify key2 is gone (should invoke factory)
    new_created = false;
    pool.get_or_create("key2", [&]() {
        new_created = true;
        return std::make_shared<InferenceSession>();
    });
    EXPECT_TRUE(new_created);
}

TEST_F(SessionPoolTest, TTLExpiration) {
    PoolConfig config;
    config.idle_timeout = std::chrono::milliseconds(100);
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<InferenceSession>(); };

    pool.get_or_create("key1", factory);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    size_t cleaned = pool.cleanup_expired();
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(SessionPoolTest, ManualEviction) {
    SessionPool pool;
    auto factory = []() { return std::make_shared<InferenceSession>(); };

    pool.get_or_create("key1", factory);
    EXPECT_EQ(pool.size(), 1);

    EXPECT_TRUE(pool.evict("key1"));
    EXPECT_EQ(pool.size(), 0);
    EXPECT_FALSE(pool.evict("key1"));
}

TEST_F(SessionPoolTest, DisableCache) {
    PoolConfig config;
    config.enable = false;
    SessionPool pool(config);

    int create_count = 0;
    auto factory = [&]() {
        create_count++;
        return std::make_shared<InferenceSession>();
    };

    auto s1 = pool.get_or_create("key1", factory);
    auto s2 = pool.get_or_create("key1", factory);

    EXPECT_NE(s1, s2);
    EXPECT_EQ(create_count, 2);
    EXPECT_EQ(pool.size(), 0);
}
