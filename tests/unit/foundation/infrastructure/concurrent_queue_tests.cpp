#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>

import foundation.infrastructure.concurrent_queue;

using namespace foundation::infrastructure;

class ConcurrentQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ConcurrentQueueTest, PushAndPop) {
    ConcurrentQueue<int> queue(10);
    queue.push(1);
    queue.push(2);

    auto val1 = queue.pop();
    ASSERT_TRUE(val1.has_value());
    EXPECT_EQ(val1.value(), 1);

    auto val2 = queue.pop();
    ASSERT_TRUE(val2.has_value());
    EXPECT_EQ(val2.value(), 2);
}

TEST_F(ConcurrentQueueTest, TryPop) {
    ConcurrentQueue<int> queue(10);
    auto empty = queue.try_pop();
    EXPECT_FALSE(empty.has_value());

    queue.push(42);
    auto val = queue.try_pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val.value(), 42);

    auto empty2 = queue.try_pop();
    EXPECT_FALSE(empty2.has_value());
}

TEST_F(ConcurrentQueueTest, SizeAndEmpty) {
    ConcurrentQueue<int> queue(10);
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);

    queue.push(1);
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);

    queue.pop();
    EXPECT_TRUE(queue.empty());
    EXPECT_EQ(queue.size(), 0);
}

TEST_F(ConcurrentQueueTest, Clear) {
    ConcurrentQueue<int> queue(10);
    queue.push(1);
    queue.push(2);
    queue.clear();
    EXPECT_TRUE(queue.empty());
}

TEST_F(ConcurrentQueueTest, BlockingPop) {
    ConcurrentQueue<int> queue(10);
    std::atomic<bool> popped{false};
    int popped_val = 0;

    std::thread t([&]() {
        auto val = queue.pop();
        if (val) {
            popped_val = *val;
            popped = true;
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(popped); // Should still be blocked

    queue.push(100);
    t.join();

    EXPECT_TRUE(popped);
    EXPECT_EQ(popped_val, 100);
}

TEST_F(ConcurrentQueueTest, BlockingPush) {
    ConcurrentQueue<int> queue(1); // Size 1
    queue.push(1);                 // Full now

    std::atomic<bool> pushed{false};

    std::thread t([&]() {
        queue.push(2); // Should block
        pushed = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(pushed); // Should still be blocked

    queue.pop(); // Make space
    t.join();

    EXPECT_TRUE(pushed);
    EXPECT_EQ(queue.size(), 1);
    auto val = queue.pop();
    EXPECT_EQ(val.value(), 2);
}

TEST_F(ConcurrentQueueTest, ShutdownWakesPop) {
    ConcurrentQueue<int> queue(10);
    std::atomic<bool> finished{false};

    std::thread t([&]() {
        auto val = queue.pop();
        if (!val) { finished = true; }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.shutdown();
    t.join();

    EXPECT_TRUE(finished);
}

TEST_F(ConcurrentQueueTest, ShutdownWakesPush) {
    ConcurrentQueue<int> queue(1);
    queue.push(1); // Full

    std::atomic<bool> finished{false};

    std::thread t([&]() {
        queue.push(2); // Should block
        // If shutdown, push returns immediately (and doesn't add)
        // Check implementation: push checks m_shutdown after wait.
        // wait predicate: m_queue.size() < m_max_size || m_shutdown
        finished = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    queue.shutdown();
    t.join();

    EXPECT_TRUE(finished);
    // Queue should still only have the first item
    EXPECT_EQ(queue.size(), 1);
}
