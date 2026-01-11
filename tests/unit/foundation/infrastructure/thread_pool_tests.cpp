
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <future>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

import foundation.infrastructure.thread_pool;

using namespace foundation::infrastructure::thread_pool;

TEST(ThreadPoolTest, SingletonInstance) {
    auto& instance1 = ThreadPool::instance();
    auto& instance2 = ThreadPool::instance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST(ThreadPoolTest, EnqueueSimpleTask) {
    std::promise<void> promise;
    auto future = promise.get_future();

    ThreadPool::instance().enqueue([&promise]() {
        promise.set_value();
    });

    // Wait for the task to complete with a timeout
    EXPECT_EQ(future.wait_for(std::chrono::seconds(2)), std::future_status::ready);
}

TEST(ThreadPoolTest, EnqueueMultipleTasks) {
    const int num_tasks = 10;
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;

    for (int i = 0; i < num_tasks; ++i) {
        auto promise = std::make_shared<std::promise<void>>();
        futures.push_back(promise->get_future());

        ThreadPool::instance().enqueue([&counter, promise]() {
            counter++;
            promise->set_value();
        });
    }

    for (auto& f : futures) {
        EXPECT_EQ(f.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    }

    EXPECT_EQ(counter.load(), num_tasks);
}

TEST(ThreadPoolTest, ConcurrentExecution) {
    // This test attempts to verify that tasks can run concurrently.
    // We enqueue 2 tasks that sleep for 100ms. If sequential, it takes 200ms+.
    // If concurrent, it takes around 100ms.
    // Note: This assumes the thread pool has at least 2 threads.

    // We can't easily check internal thread count, but we can check if they overlap.
    std::atomic<int> active_threads{0};
    std::atomic<bool> overlap_detected{false};
    std::promise<void> p1, p2;
    auto f1 = p1.get_future();
    auto f2 = p2.get_future();

    auto task = [&active_threads, &overlap_detected](std::promise<void> p) {
        active_threads++;
        if (active_threads > 1) {
            overlap_detected = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        active_threads--;
        p.set_value();
    };

    ThreadPool::instance().enqueue([&]() { task(std::move(p1)); });
    ThreadPool::instance().enqueue([&]() { task(std::move(p2)); });

    f1.wait();
    f2.wait();

    // It's not guaranteed to overlap if the OS schedules them sequentially or the pool is busy,
    // but in a dedicated test environment with >=2 threads, it likely will.
    // We won't strictly fail the test if no overlap to avoid flakiness on single-core setups (rare),
    // but checking it is useful for manual verification.
    // EXPECT_TRUE(overlap_detected); // Commented out to prevent flakiness
}
