
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include <string>
#include <stdexcept>

import foundation.ai.inference_session;

using namespace foundation::ai::inference_session;

class InferenceSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // No special setup needed for now
    }
};

TEST_F(InferenceSessionTest, Initialization) {
    EXPECT_NO_THROW({ InferenceSession session; });
}

TEST_F(InferenceSessionTest, IsModelLoaded) {
    InferenceSession session;
    EXPECT_FALSE(session.is_model_loaded());
    EXPECT_EQ(session.get_loaded_model_path(), "");
}

TEST_F(InferenceSessionTest, IdempotentLoading) {
    // Note: Logic verified in model_registry_tests.cpp
}
