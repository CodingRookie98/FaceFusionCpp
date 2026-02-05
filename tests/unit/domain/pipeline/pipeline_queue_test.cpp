#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

import domain.pipeline;

using namespace domain::pipeline;
using namespace std::chrono_literals;

class ThreadSafeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

// --- Basic Functionality Tests ---

TEST_F(ThreadSafeQueueTest, PushAndPopSingleItem) {
    ThreadSafeQueue<int> queue(10);
    queue.push(42);

    EXPECT_EQ(queue.size(), 1);
    EXPECT_FALSE(queue.empty());

    auto val = queue.pop();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42);
    EXPECT_TRUE(queue.empty());
}

TEST_F(ThreadSafeQueueTest, PopBatchItems) {
    ThreadSafeQueue<int> queue(10);
    for (int i = 0; i < 5; ++i) queue.push(i);

    auto batch = queue.pop_batch(3);
    ASSERT_EQ(batch.size(), 3);
    EXPECT_EQ(batch[0], 0);
    EXPECT_EQ(batch[1], 1);
    EXPECT_EQ(batch[2], 2);
    EXPECT_EQ(queue.size(), 2);

    auto batch2 = queue.pop_batch(10);
    EXPECT_EQ(batch2.size(), 2);
    EXPECT_TRUE(queue.empty());
}

// --- Blocking and Concurrency Tests ---

TEST_F(ThreadSafeQueueTest, PopBlocksUntilDataAvailable) {
    ThreadSafeQueue<int> queue(10);
    std::atomic<int> result(0);
    std::atomic<bool> popped(false);

    std::thread t([&] {
        auto val = queue.pop();
        if (val) {
            result = *val;
            popped = true;
        }
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(popped);

    queue.push(123);
    t.join();

    EXPECT_TRUE(popped);
    EXPECT_EQ(result, 123);
}

TEST_F(ThreadSafeQueueTest, PushBlocksWhenFull) {
    ThreadSafeQueue<int> queue(2);
    queue.push(1);
    queue.push(2);

    std::atomic<bool> pushed_third(false);
    std::thread t([&] {
        queue.push(3);
        pushed_third = true;
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_FALSE(pushed_third);

    auto val = queue.pop();
    EXPECT_EQ(*val, 1);

    t.join();
    EXPECT_TRUE(pushed_third);
    EXPECT_EQ(queue.size(), 2);
}

// --- Shutdown Tests ---

TEST_F(ThreadSafeQueueTest, ShutdownWakesUpPoppers) {
    ThreadSafeQueue<int> queue(10);
    std::atomic<bool> pop_returned(false);

    std::thread t([&] {
        auto val = queue.pop();
        EXPECT_FALSE(val.has_value());
        pop_returned = true;
    });

    std::this_thread::sleep_for(50ms);
    queue.shutdown();
    t.join();

    EXPECT_TRUE(pop_returned);
    EXPECT_FALSE(queue.is_active());
}

TEST_F(ThreadSafeQueueTest, ShutdownWakesUpPushers) {
    ThreadSafeQueue<int> queue(1);
    queue.push(1);

    std::atomic<bool> push_returned(false);
    std::thread t([&] {
        queue.push(2);
        push_returned = true;
    });

    std::this_thread::sleep_for(50ms);
    queue.shutdown();
    t.join();

    EXPECT_TRUE(push_returned);
}

TEST_F(ThreadSafeQueueTest, PopBatchAfterShutdownReturnsRemaining) {
    ThreadSafeQueue<int> queue(10);
    queue.push(1);
    queue.push(2);
    queue.shutdown();

    auto batch = queue.pop_batch(10);
    EXPECT_EQ(batch.size(), 2);

    auto batch2 = queue.pop_batch(10);
    EXPECT_TRUE(batch2.empty());
}

// --- Stress Test ---

TEST_F(ThreadSafeQueueTest, MultiProducerMultiConsumer) {
    const int num_producers = 4;
    const int num_consumers = 4;
    const int items_per_producer = 1000;
    ThreadSafeQueue<int> queue(100);
    std::atomic<int> total_consumed(0);
    std::atomic<int> sum_consumed(0);

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&, i] {
            for (int j = 0; j < items_per_producer; ++j) { queue.push(1); }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&] {
            while (true) {
                auto val = queue.pop();
                if (!val) break;
                total_consumed++;
                sum_consumed += *val;
            }
        });
    }

    for (auto& t : producers) t.join();
    queue.shutdown();
    for (auto& t : consumers) t.join();

    EXPECT_EQ(total_consumed, num_producers * items_per_producer);
    EXPECT_EQ(sum_consumed, num_producers * items_per_producer);
}
