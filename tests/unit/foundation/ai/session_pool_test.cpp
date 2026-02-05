#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <thread>
#include <chrono>
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
