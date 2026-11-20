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

// ---- T3: session key 包含模型文件指纹（热更新失效修复） ----

TEST_F(InferenceSessionRegistryTest, KeyChangesWhenFileMtimeChanges) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;

    std::string model_path = (fs::path(temp_dir) / "model.onnx").string();
    {
        std::ofstream f(model_path, std::ios::binary);
        f.write("0123456789", 10);
    }
    const auto original_time = fs::last_write_time(model_path);

    const std::string key1 = registry->generate_key(model_path, opts);

    // 修改文件内容并更新 mtime
    {
        std::ofstream f(model_path, std::ios::binary | std::ios::trunc);
        f.write("9876543210", 10);
    }
    fs::last_write_time(model_path, original_time + std::chrono::seconds(5));

    const std::string key2 = registry->generate_key(model_path, opts);

    EXPECT_NE(key1, key2) << "模型文件 mtime 变化后 key 应变化（热更新生效）";
}

TEST_F(InferenceSessionRegistryTest, KeyStableForUnchangedFile) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;

    std::string model_path = (fs::path(temp_dir) / "stable.onnx").string();
    {
        std::ofstream f(model_path, std::ios::binary);
        f.write("0123456789", 10);
    }

    const std::string key1 = registry->generate_key(model_path, opts);
    const std::string key2 = registry->generate_key(model_path, opts);

    EXPECT_EQ(key1, key2);
}

TEST_F(InferenceSessionRegistryTest, KeyIncludesFileSize) {
    auto registry = InferenceSessionRegistry::get_instance();
    Options opts;

    std::string model_path = (fs::path(temp_dir) / "sized.onnx").string();
    {
        std::ofstream f(model_path, std::ios::binary);
        f.write("0123456789", 10);
    }
    const auto original_time = fs::last_write_time(model_path);

    const std::string key1 = registry->generate_key(model_path, opts);

    // 改变文件大小但保持 mtime 不变 → size 应参与指纹
    {
        std::ofstream f(model_path, std::ios::binary | std::ios::trunc);
        f.write("0123456789ABCDEF", 16);
    }
    fs::last_write_time(model_path, original_time);

    const std::string key2 = registry->generate_key(model_path, opts);

    EXPECT_NE(key1, key2) << "文件 size 变化后 key 应变化";
}
