
/**
 * @file progress_tests.cpp
 * @brief Unit tests for progress indicators.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

import foundation.infrastructure.progress;

using namespace foundation::infrastructure::progress;

TEST(ProgressTest, ProgressBarLifecycle) {
    EXPECT_NO_THROW({
        ProgressBar pb;
        EXPECT_FALSE(pb.is_completed());
        pb.set_progress(50.0f);
        pb.set_postfix_text("Processing...");
        pb.tick();
        pb.mark_as_completed();
        EXPECT_TRUE(pb.is_completed());
    });
}

TEST(ProgressTest, ProgressUpdate) {
    ProgressBar pb;
    pb.set_progress(10.0f);
    // Ideally we would mock the underlying indicator, but with PIMPL it's hard.
    // We just ensure it doesn't crash.
    for (int i = 0; i < 5; ++i) { pb.tick(); }
    pb.mark_as_completed();
    EXPECT_TRUE(pb.is_completed());
}
