#include <gtest/gtest.h>
#include <cstdint>

import services.pipeline.runner;

using namespace services::pipeline;

// T4: Strict/普通路径 checkpoint 保存节流（每 100 帧）

TEST(CheckpointThrottleTest, DoesNotSaveBeforeFirstHundred) {
    EXPECT_FALSE(should_save_checkpoint(0));
    EXPECT_FALSE(should_save_checkpoint(1));
    EXPECT_FALSE(should_save_checkpoint(50));
    EXPECT_FALSE(should_save_checkpoint(99));
}

TEST(CheckpointThrottleTest, SavesEveryHundredFrames) {
    EXPECT_TRUE(should_save_checkpoint(100));
    EXPECT_TRUE(should_save_checkpoint(200));
    EXPECT_TRUE(should_save_checkpoint(1000));
}

TEST(CheckpointThrottleTest, DoesNotSaveOnNonMultiples) {
    EXPECT_FALSE(should_save_checkpoint(101));
    EXPECT_FALSE(should_save_checkpoint(250));
    EXPECT_FALSE(should_save_checkpoint(999));
}