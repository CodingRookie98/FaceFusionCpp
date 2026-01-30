
/**
 * @file inference_session_tests.cpp
 * @brief Unit tests for InferenceSession.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_set>
#include <string>
#include <stdexcept>
#include <filesystem>

import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace foundation::ai::inference_session;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class InferenceSessionTest : public ::testing::Test {
protected:
    void SetUp() override {
        assets_path = get_assets_path();
        // Use yoloface_8n.onnx as a lightweight test model if available
        test_model_path = assets_path / "models/yoloface_8n.onnx";
    }

    fs::path assets_path;
    fs::path test_model_path;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(InferenceSessionTest, Initialization) {
    EXPECT_NO_THROW({ InferenceSession session; });
}

TEST_F(InferenceSessionTest, InitialState) {
    InferenceSession session;
    EXPECT_FALSE(session.is_model_loaded());
    EXPECT_EQ(session.get_loaded_model_path(), "");
    EXPECT_TRUE(session.get_input_names().empty());
    EXPECT_TRUE(session.get_output_names().empty());
}

// ============================================================================
// Options Tests
// ============================================================================

TEST_F(InferenceSessionTest, OptionsEquality) {
    Options opt1;
    opt1.execution_providers = {ExecutionProvider::CPU};
    opt1.execution_device_id = 0;

    Options opt2;
    opt2.execution_providers = {ExecutionProvider::CPU};
    opt2.execution_device_id = 0;

    EXPECT_TRUE(opt1 == opt2);

    opt2.execution_device_id = 1;
    EXPECT_FALSE(opt1 == opt2);
}

TEST_F(InferenceSessionTest, OptionsWithBestProviders) {
    auto opts = Options::with_best_providers();
    EXPECT_FALSE(opts.execution_providers.empty());
    // Should contain at least CPU
    EXPECT_TRUE(opts.execution_providers.contains(ExecutionProvider::CPU));
}

// ============================================================================
// Model Loading Tests
// ============================================================================

TEST_F(InferenceSessionTest, LoadModelThrowsOnInvalidPath) {
    InferenceSession session;
    Options opts;
    EXPECT_ANY_THROW(session.load_model("non_existent_model.onnx", opts));
    EXPECT_FALSE(session.is_model_loaded());
}

TEST_F(InferenceSessionTest, LoadModelSuccess) {
    if (!fs::exists(test_model_path)) {
        GTEST_SKIP() << "Test model not found at: " << test_model_path;
    }

    InferenceSession session;
    Options opts;
    // Use CPU for stable testing
    opts.execution_providers = {ExecutionProvider::CPU};

    EXPECT_NO_THROW(session.load_model(test_model_path.string(), opts));

    EXPECT_TRUE(session.is_model_loaded());
    EXPECT_EQ(session.get_loaded_model_path(), test_model_path.string());

    // Verify model metadata (YoloFace specific)
    auto input_names = session.get_input_names();
    EXPECT_FALSE(input_names.empty());

    auto output_names = session.get_output_names();
    EXPECT_FALSE(output_names.empty());
}

TEST_F(InferenceSessionTest, ReloadModel) {
    if (!fs::exists(test_model_path)) { GTEST_SKIP() << "Test model not found"; }

    InferenceSession session;
    Options opts;
    opts.execution_providers = {ExecutionProvider::CPU};

    // Load first time
    session.load_model(test_model_path.string(), opts);
    EXPECT_TRUE(session.is_model_loaded());

    // Reload same model (should be no-op or fast)
    EXPECT_NO_THROW(session.load_model(test_model_path.string(), opts));
    EXPECT_TRUE(session.is_model_loaded());
}
