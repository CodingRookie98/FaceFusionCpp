/**
 * @file scoped_timer_test.cpp
 * @brief Unit tests for ScopedTimer
 * @date 2026-02-05
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

import foundation.infrastructure.scoped_timer;

using namespace foundation::infrastructure;

TEST(ScopedTimerTest, ConstructWithOperationName) {
    EXPECT_NO_THROW({
        ScopedTimer timer("TestOperation");
        EXPECT_GE(timer.elapsed_seconds(), 0.0);
    });
}

TEST(ScopedTimerTest, ConstructWithEntryParams) {
    EXPECT_NO_THROW({
        ScopedTimer timer("TestOperation", "param1=value1");
        EXPECT_GE(timer.elapsed_seconds(), 0.0);
    });
}

TEST(ScopedTimerTest, ElapsedTimeIncreases) {
    ScopedTimer timer("TestDelay");
    auto start = timer.elapsed();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto end = timer.elapsed();
    EXPECT_GT(end, start);
    // Allow for some system jitter, but generally should be >= 10ms
    // We use a loose check to avoid flaky tests on busy systems
    EXPECT_GE(end, std::chrono::milliseconds(5));
}

TEST(ScopedTimerTest, ElapsedSeconds) {
    ScopedTimer timer("TestDelaySeconds");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_GE(timer.elapsed_seconds(), 0.005);
}

TEST(ScopedTimerTest, Checkpoint) {
    ScopedTimer timer("TestCheckpoint");
    EXPECT_NO_THROW(timer.checkpoint("step1"));
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    EXPECT_NO_THROW(timer.checkpoint("step2"));
}

TEST(ScopedTimerTest, SetResult) {
    ScopedTimer timer("TestResult");
    EXPECT_NO_THROW(timer.set_result("Success"));
}

/* Macros cannot be tested via import as they are not exported
TEST(ScopedTimerTest, Macros) {
    EXPECT_NO_THROW({
        SCOPED_TIMER();
    });
    EXPECT_NO_THROW({
        SCOPED_TIMER_NAMED("MacroTest");
    });
}
*/
