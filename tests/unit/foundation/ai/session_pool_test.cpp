#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
#include <future>
#include <onnxruntime_cxx_api.h>

import foundation.ai.session_pool;
import foundation.ai.inference_session;

using namespace foundation::ai::session_pool;
using namespace foundation::ai::inference_session;

// Mock class
class MockInferenceSession : public InferenceSession {
public:
    MOCK_METHOD(void, load_model, (const std::string&, const Options&), (override));
    // Note: Mocking methods with complex types like Ort::Value might require care,
    // but since we are not calling run() in these tests, it's fine.
    MOCK_METHOD(std::vector<Ort::Value>, run, (const std::vector<Ort::Value>&), (override));
};

class SessionPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SessionPoolTest, GetOrCreateCreatesNewSession) {
    SessionPool pool;
    int factory_calls = 0;
    auto factory = [&]() {
        factory_calls++;
        return std::make_shared<MockInferenceSession>();
    };

    auto session1 = pool.get_or_create("key1", factory);
    ASSERT_NE(session1, nullptr);
    EXPECT_EQ(factory_calls, 1);
}

TEST_F(SessionPoolTest, GetOrCreateReturnsCachedSession) {
    SessionPool pool;
    int factory_calls = 0;
    auto factory = [&]() {
        factory_calls++;
        return std::make_shared<MockInferenceSession>();
    };

    auto session1 = pool.get_or_create("key1", factory);
    auto session2 = pool.get_or_create("key1", factory);

    ASSERT_EQ(session1, session2);
    EXPECT_EQ(factory_calls, 1);
}

TEST_F(SessionPoolTest, LRUEviction) {
    PoolConfig config;
    config.max_entries = 2;
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    // Fill pool
    pool.get_or_create("key1", factory);
    pool.get_or_create("key2", factory);
    EXPECT_EQ(pool.size(), 2);

    // Access key1 to make it recently used (key2 becomes LRU)
    pool.get_or_create("key1", factory);

    // Add key3, should evict key2
    pool.get_or_create("key3", factory);

    EXPECT_EQ(pool.size(), 2);
    auto stats = pool.get_stats();
    EXPECT_EQ(stats.evictions, 1);

    // Verify key1 is still there (factory NOT called)
    // We check key1 FIRST to avoid evicting it if we were to insert key2 first
    int key1_calls = 0;
    pool.get_or_create("key1", [&]() {
        key1_calls++;
        return std::make_shared<MockInferenceSession>();
    });
    EXPECT_EQ(key1_calls, 0);

    // Verify key2 is gone (factory called again)
    int key2_calls = 0;
    pool.get_or_create("key2", [&]() {
        key2_calls++;
        return std::make_shared<MockInferenceSession>();
    });
    EXPECT_EQ(key2_calls, 1);
}

TEST_F(SessionPoolTest, TTLExpiration) {
    PoolConfig config;
    config.idle_timeout = std::chrono::milliseconds(50);
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("key1", factory);
    EXPECT_EQ(pool.size(), 1);

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Cleanup
    size_t removed = pool.cleanup_expired();
    EXPECT_EQ(removed, 1);
    EXPECT_EQ(pool.size(), 0);

    auto stats = pool.get_stats();
    EXPECT_EQ(stats.expirations, 1);
}

TEST_F(SessionPoolTest, ManualEviction) {
    SessionPool pool;
    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("key1", factory);
    EXPECT_EQ(pool.size(), 1);

    EXPECT_TRUE(pool.evict("key1"));
    EXPECT_EQ(pool.size(), 0);
    EXPECT_FALSE(pool.evict("key1"));
}

TEST_F(SessionPoolTest, Clear) {
    SessionPool pool;
    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("key1", factory);
    pool.get_or_create("key2", factory);
    EXPECT_EQ(pool.size(), 2);

    pool.clear();
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(SessionPoolTest, DisableCaching) {
    PoolConfig config;
    config.enable = false;
    SessionPool pool(config);

    int factory_calls = 0;
    auto factory = [&]() {
        factory_calls++;
        return std::make_shared<MockInferenceSession>();
    };

    pool.get_or_create("key1", factory);
    pool.get_or_create("key1", factory);

    EXPECT_EQ(factory_calls, 2); // Should be called every time
    EXPECT_EQ(pool.size(), 0);
}

TEST_F(SessionPoolTest, DefaultCapacitySupportsSixSessions) {
    // 默认容量必须 >= 单任务模型组合数（detector+landmarker+swapper+enhancer+occlusion+region = 6）
    SessionPool pool;
    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    for (int i = 1; i <= 6; ++i) { pool.get_or_create("key" + std::to_string(i), factory); }

    EXPECT_EQ(pool.size(), 6);
    EXPECT_EQ(pool.get_stats().evictions, 0);
}

TEST_F(SessionPoolTest, ExplicitSmallCapacityStillEvicts) {
    PoolConfig config;
    config.max_entries = 2;
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("key1", factory);
    pool.get_or_create("key2", factory);
    pool.get_or_create("key3", factory);

    EXPECT_EQ(pool.size(), 2);
    EXPECT_EQ(pool.get_stats().evictions, 1);
}
TEST_F(SessionPoolTest, ExplicitLargeCapacityHonored) {
    PoolConfig config;
    config.max_entries = 20;
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    for (int i = 1; i <= 10; ++i) { pool.get_or_create("key" + std::to_string(i), factory); }

    EXPECT_EQ(pool.size(), 10);
    EXPECT_EQ(pool.get_stats().evictions, 0);
}

// ---- T4: TTL 惰性清理（get_or_create 周期触发） ----

TEST_F(SessionPoolTest, LazyCleanupExpiresIdleSessions) {
    PoolConfig config;
    config.idle_timeout = std::chrono::milliseconds(50);
    config.cleanup_interval = std::chrono::milliseconds(0); // 每次 get 都清理
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("idle_key", factory);
    EXPECT_EQ(pool.size(), 1);

    // 等待 idle_key 过期
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 触发惰性清理（新 key 的 get_or_create）
    pool.get_or_create("new_key", factory);

    EXPECT_EQ(pool.get_stats().expirations, 1);
    EXPECT_EQ(pool.size(), 1);
    EXPECT_EQ(pool.get_stats().hits + pool.get_stats().misses, 2); // new_key 为 miss
}

TEST_F(SessionPoolTest, LazyCleanupKeepsFreshSessions) {
    PoolConfig config;
    config.idle_timeout = std::chrono::seconds(10);
    config.cleanup_interval = std::chrono::milliseconds(0);
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("fresh_key", factory);
    pool.get_or_create("another_key", factory);

    EXPECT_EQ(pool.size(), 2);
    EXPECT_EQ(pool.get_stats().expirations, 0);
}

TEST_F(SessionPoolTest, LazyCleanupThrottledByInterval) {
    PoolConfig config;
    config.idle_timeout = std::chrono::milliseconds(50);
    config.cleanup_interval = std::chrono::seconds(10); // 长间隔节流
    SessionPool pool(config);

    auto factory = []() { return std::make_shared<MockInferenceSession>(); };

    pool.get_or_create("idle_key", factory);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 清理被节流：idle_key 虽过期但不触发清理
    pool.get_or_create("new_key", factory);

    EXPECT_EQ(pool.get_stats().expirations, 0);
    EXPECT_EQ(pool.size(), 2);
}

// ---- T3: 池锁重构（factory 移出锁） ----

TEST_F(SessionPoolTest, FactoryRunsOutsideLock) {
    SessionPool pool;
    auto factory = [&pool]() -> std::shared_ptr<MockInferenceSession> {
        // 若 factory 在池锁内执行，size()（需拿锁）会死锁（非递归 mutex）
        (void)pool.size();
        return std::make_shared<MockInferenceSession>();
    };

    auto fut =
        std::async(std::launch::async, [&]() { return pool.get_or_create("key1", factory); });
    EXPECT_NE(fut.wait_for(std::chrono::seconds(2)), std::future_status::timeout)
        << "factory 内访问池导致死锁 = factory 仍在锁内执行";
    auto session = fut.get();
    ASSERT_NE(session, nullptr);
}

TEST_F(SessionPoolTest, ConcurrentSameKeySingleCreation) {
    SessionPool pool;
    std::atomic<int> factory_calls{0};
    std::atomic<int> active{0};
    std::atomic<int> peak{0};
    auto factory = [&]() {
        const int cur = ++active;
        int m = peak.load();
        while (cur > m && !peak.compare_exchange_weak(m, cur)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --active;
        factory_calls++;
        return std::make_shared<MockInferenceSession>();
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&]() { pool.get_or_create("same_key", factory); });
    }
    for (auto& t : threads) { t.join(); }

    EXPECT_EQ(factory_calls, 1) << "并发同 key 应只创建一次";
    EXPECT_EQ(pool.size(), 1);
}

TEST_F(SessionPoolTest, ConcurrentDistinctKeysAllCreated) {
    SessionPool pool;
    std::atomic<int> factory_calls{0};
    auto factory = [&]() {
        factory_calls++;
        return std::make_shared<MockInferenceSession>();
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&, i]() { pool.get_or_create("key" + std::to_string(i), factory); });
    }
    for (auto& t : threads) { t.join(); }

    EXPECT_EQ(factory_calls, 8);
    EXPECT_EQ(pool.size(), 8);
}