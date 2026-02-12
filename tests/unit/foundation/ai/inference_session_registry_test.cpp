#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "common/test_paths.h"

import foundation.ai.inference_session_registry;
import foundation.ai.inference_session;

using namespace foundation::ai::inference_session;
namespace fs = std::filesystem;

class InferenceSessionRegistryTest : public ::testing::Test {
protected:
    std::string temp_dir;

    void SetUp() override {
        auto base_dir = tests::common::TestPaths::GetTestOutputDir("inference_session_registry");
        temp_dir = (base_dir / ("test_temp_registry_" + std::to_string(std::rand()))).string();
        fs::create_directories(temp_dir);
    }

    void TearDown() override {
        if (fs::exists(temp_dir)) { fs::remove_all(temp_dir); }
        // Clear registry to avoid side effects
        InferenceSessionRegistry::get_instance()->clear();
    }
};

TEST_F(InferenceSessionRegistryTest, SingletonInstance) {
    auto instance1 = InferenceSessionRegistry::get_instance();
    auto instance2 = InferenceSessionRegistry::get_instance();
    EXPECT_EQ(instance1, instance2);
}

TEST_F(InferenceSessionRegistryTest, GetSessionThrowsIfModelNotFound) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;
    EXPECT_THROW(registry->get_session("non_existent_model.onnx", opts), std::runtime_error);
}

TEST_F(InferenceSessionRegistryTest, GetSessionThrowsIfModelInvalid) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;

    // Create an empty file (invalid ONNX)
    std::string model_path = (fs::path(temp_dir) / "invalid.onnx").string();
    std::ofstream(model_path).close();

    // ONNX Runtime should throw when trying to load an empty/invalid file
    // Note: The specific exception type might depend on ONNX Runtime wrapper,
    // but code says it throws std::exception derived (Ort::Exception) or runtime_error.
    EXPECT_ANY_THROW(registry->get_session(model_path, opts));
}

TEST_F(InferenceSessionRegistryTest, CleanupExpired) {
    // We can't easily test cleanup logic without successful session creation
    // because we can't insert into the pool without a valid model.
    // So we just verify the method exists and runs without crashing on empty pool.
    auto registry = InferenceSessionRegistry::get_instance();
    EXPECT_EQ(registry->cleanup_expired(), 0);
}
