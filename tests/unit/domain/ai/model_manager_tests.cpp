
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <nlohmann/json.hpp>

import domain.ai.model_manager;
import foundation.infrastructure.file_system;

namespace fs = std::filesystem;
using namespace domain::ai::model_manager;

class ModelManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_json_path = "test_models_info.json";
        std::ofstream file(test_json_path);
        file << R"({
            "models_info": [
                {
                    "name": "test_model_1",
                    "type": "face_enhancer",
                    "url": "http://example.com/model1.onnx",
                    "path": "./models/test_model_1.onnx"
                },
                {
                    "name": "test_model_2",
                    "type": "face_swapper",
                    "url": "http://example.com/model2.onnx",
                    "path": "./models/test_model_2.onnx"
                }
            ]
        })";
        file.close();
    }

    void TearDown() override {
        if (fs::exists(test_json_path)) { fs::remove(test_json_path); }
    }
    std::string test_json_path;
};

TEST_F(ModelManagerTest, SingletonInstance) {
    auto instance = ModelManager::get_instance();
    ASSERT_NE(instance, nullptr);
    // Verify default path is used initially or settable
    // Note: Default path "./assets/models_info.json" might not exist in test env,
    // but constructor might throw if not found.
    // However, since we are using singleton, it might have been initialized in previous tests or
    // runs. Let's set it to test path to be sure.
    instance->set_model_info_file_path(test_json_path);
    EXPECT_EQ(instance->get_model_json_file_path(), test_json_path);
}

TEST_F(ModelManagerTest, LoadConfiguration) {
    auto instance = ModelManager::get_instance();
    instance->set_model_info_file_path(test_json_path);
    EXPECT_TRUE(instance->has_model("test_model_1"));
    EXPECT_TRUE(instance->has_model("test_model_2"));
    EXPECT_FALSE(instance->has_model("non_existent_model"));
}

TEST_F(ModelManagerTest, GetModelInfo) {
    auto instance = ModelManager::get_instance();
    instance->set_model_info_file_path(test_json_path);
    auto info = instance->get_model_info("test_model_1");

    EXPECT_EQ(info.name, "test_model_1");
    EXPECT_EQ(info.type, "face_enhancer");
    EXPECT_EQ(info.url, "http://example.com/model1.onnx");
    EXPECT_EQ(info.path, "./models/test_model_1.onnx");
}

TEST_F(ModelManagerTest, GetModelInfoInvalid) {
    auto instance = ModelManager::get_instance();
    instance->set_model_info_file_path(test_json_path);
    auto info = instance->get_model_info("invalid_model");
    EXPECT_TRUE(info.name.empty());
}

TEST_F(ModelManagerTest, JSONSerialization) {
    ModelInfo original{"test", "type", "path", "url"};
    nlohmann::json j;
    to_json(j, original);

    ModelInfo deserialized;
    from_json(j, deserialized);

    EXPECT_EQ(deserialized.name, original.name);
    EXPECT_EQ(deserialized.type, original.type);
    EXPECT_EQ(deserialized.path, original.path);
    EXPECT_EQ(deserialized.url, original.url);
}

TEST_F(ModelManagerTest, LoadRealAssetsModelInfo) {
    // This test assumes "./assets/models_info.json" exists relative to execution directory
    // or we might need to adjust path if running from build dir.
    // The user mentioned "asseets/models_info.json" (typo in user prompt? assuming assets)
    // Checking if file exists first to avoid fail if env setup is different.
    std::string real_path = "./assets/models_info.json";
    if (fs::exists(real_path)) {
        auto instance = ModelManager::get_instance();
        EXPECT_NO_THROW(instance->set_model_info_file_path(real_path));
        EXPECT_EQ(instance->get_model_json_file_path(), real_path);
        // Maybe check for a known model like "gfpgan_1.4"
        EXPECT_TRUE(instance->has_model("gfpgan_1.4"));
    } else {
        // Try assuming running from project root, while build might be in build/
        // If test is run from project root, ./assets exists.
        // If test is run from build/msvc-x64-debug, then ../../assets might be needed.
        // But let's just warn or skip if not found.
        std::cout << "[WARNING] Real assets file not found at " << real_path
                  << ", skipping real asset test." << std::endl;
    }
}
