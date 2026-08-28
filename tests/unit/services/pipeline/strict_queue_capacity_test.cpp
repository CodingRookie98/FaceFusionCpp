#include <gtest/gtest.h>

import services.pipeline.runner;

using namespace services::pipeline;

// T1: Strict 内存模式 Pipeline 帧队列上限（默认 4 → 16，缓解流水线吞吐受限）

TEST(StrictQueueCapacityTest, DefaultsToSixteenWhenUnconfigured) {
    EXPECT_EQ(strict_queue_limit(0), 16);
    EXPECT_EQ(strict_queue_limit(-1), 16);
}

TEST(StrictQueueCapacityTest, CapsConfiguredAboveSixteen) {
    EXPECT_EQ(strict_queue_limit(20), 16);
    EXPECT_EQ(strict_queue_limit(100), 16);
}

TEST(StrictQueueCapacityTest, PreservesSmallerConfiguredValue) {
    EXPECT_EQ(strict_queue_limit(4), 4);
    EXPECT_EQ(strict_queue_limit(8), 8);
}