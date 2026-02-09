/**
 * @file inference_session_registry_tests.cpp
 * @brief Unit tests for
 * InferenceSessionRegistry.
 * @author CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <filesystem>

import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import foundation.infrastructure.test_support;

using namespace foundation::ai::inference_session;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class InferenceSessionRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure registry is clean
        InferenceSessionRegistry::get_instance()->clear();

        assets_path = get_assets_path();
        test_model_path = assets_path / "models/yoloface_8n.onnx";
    }

    fs::path assets_path;
    fs::path test_model_path;
};

TEST_F(InferenceSessionRegistryTest, SingletonInstance) {
    auto registry1 = InferenceSessionRegistry::get_instance();
    auto registry2 = InferenceSessionRegistry::get_instance();
    EXPECT_EQ(registry1, registry2);
}

TEST_F(InferenceSessionRegistryTest, GetSessionInvalidPath) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;

    // Should throw if model file doesn't exist
    EXPECT_ANY_THROW(registry->get_session("non_existent_model.onnx", opts));
}

TEST_F(InferenceSessionRegistryTest, GetSessionReuse) {
    if (!fs::exists(test_model_path)) {
        GTEST_SKIP() << "Test model not found: " << test_model_path;
    }

    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;
    opts.execution_providers = {ExecutionProvider::CPU};

    // 1. Create first session
    auto session1 = registry->get_session(test_model_path.string(), opts);
    ASSERT_NE(session1, nullptr);
    EXPECT_TRUE(session1->is_model_loaded());

    // 2. Get same session (same path, same options)
    auto session2 = registry->get_session(test_model_path.string(), opts);
    EXPECT_EQ(session1, session2) << "Registry should return same session instance";

    // 3. Different options -> Different session
    Options opts2 = opts;
    opts2.execution_device_id = 999; // Artificial difference

    // Change a non-critical option to force a different session key.
    // enable_tensorrt_cache
    opts2.enable_tensorrt_cache = !opts.enable_tensorrt_cache;
    opts2.execution_device_id = 0; // Restore valid device ID

    auto session3 = registry->get_session(test_model_path.string(), opts2);
    EXPECT_NE(session1, session3) << "Different options should create new session";
}

TEST_F(InferenceSessionRegistryTest, ClearRegistry) {
    if (!fs::exists(test_model_path)) { GTEST_SKIP() << "Test model not found"; }

    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;
    opts.execution_providers = {ExecutionProvider::CPU};

    auto session1 = registry->get_session(test_model_path.string(), opts);
    ASSERT_NE(session1, nullptr);

    registry->clear();

    // After clear, getting same model should produce NEW session instance
    auto session2 = registry->get_session(test_model_path.string(), opts);
    EXPECT_NE(session1, session2) << "Registry should create new session after clear";
}
