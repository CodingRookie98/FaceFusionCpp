
/**
 * @file progress_tests.cpp
 * @brief Unit tests for progress indicators.
 * @author
 * CodingRookie
 * @date 2026-02-05
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

import foundation.infrastructure.progress;

using namespace foundation::infrastructure::progress;

TEST(ProgressTest, DefaultConstructor) {
    EXPECT_NO_THROW({
        ProgressBar pb;
        EXPECT_FALSE(pb.is_completed());
        pb.mark_as_completed();
        EXPECT_TRUE(pb.is_completed());
    });
}

TEST(ProgressTest, ConstructorWithCustomText) {
    EXPECT_NO_THROW({
        ProgressBar pb("Initializing...");
        EXPECT_FALSE(pb.is_completed());
        pb.mark_as_completed();
        EXPECT_TRUE(pb.is_completed());
    });
}

TEST(ProgressTest, SetProgress) {
    ProgressBar pb;
    EXPECT_NO_THROW(pb.set_progress(50.0f));
    EXPECT_NO_THROW(pb.set_progress(100.0f));
}

TEST(ProgressTest, SetPostfixText) {
    ProgressBar pb;
    EXPECT_NO_THROW(pb.set_postfix_text("Loading resources..."));
    EXPECT_NO_THROW(pb.set_postfix_text("Processing frame 1/100"));
}

TEST(ProgressTest, Tick) {
    ProgressBar pb;
    EXPECT_NO_THROW(pb.tick());
    EXPECT_NO_THROW(pb.tick());
}

TEST(ProgressTest, CompletionStatus) {
    ProgressBar pb;
    EXPECT_FALSE(pb.is_completed());

    pb.set_progress(100.0f);
    // Note: set_progress(100.0f) does NOT automatically mark as completed in the wrapper logic
    // unless the underlying library does so, but our wrapper exposes mark_as_completed explicitly.
    // Let's verify explicit completion.

    pb.mark_as_completed();
    EXPECT_TRUE(pb.is_completed());
}

TEST(ProgressTest, LifecycleStress) {
    EXPECT_NO_THROW({
        ProgressBar pb("Stress Test");
        for (int i = 0; i < 100; ++i) {
            pb.set_progress(static_cast<float>(i));
            pb.set_postfix_text("Step " + std::to_string(i));
            pb.tick();
        }
        pb.mark_as_completed();
    });
}
