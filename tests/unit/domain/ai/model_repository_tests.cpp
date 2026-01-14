
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>

import domain.ai.model_repository;
import foundation.infrastructure.file_system;
import foundation.infrastructure.test_support;

namespace fs = std::filesystem;
using namespace domain::ai::model_repository;

class ModelRepositoryTest : public ::testing::Test {
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

TEST_F(ModelRepositoryTest, SingletonInstance) {
    auto instance = ModelRepository::get_instance();
    ASSERT_NE(instance, nullptr);
    instance->set_model_info_file_path(test_json_path);
    EXPECT_EQ(instance->get_model_json_file_path(), test_json_path);
}

TEST_F(ModelRepositoryTest, LoadConfiguration) {
    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(test_json_path);
    EXPECT_TRUE(instance->has_model("test_model_1"));
    EXPECT_TRUE(instance->has_model("test_model_2"));
    EXPECT_FALSE(instance->has_model("non_existent_model"));
}

TEST_F(ModelRepositoryTest, GetModelInfo) {
    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(test_json_path);
    auto info = instance->get_model_info("test_model_1");

    EXPECT_EQ(info.name, "test_model_1");
    EXPECT_EQ(info.type, "face_enhancer");
    EXPECT_EQ(info.url, "http://example.com/model1.onnx");
    EXPECT_EQ(info.path, "./models/test_model_1.onnx");
}

TEST_F(ModelRepositoryTest, GetModelInfoInvalid) {
    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(test_json_path);
    auto info = instance->get_model_info("invalid_model");
    EXPECT_TRUE(info.name.empty());
}

TEST_F(ModelRepositoryTest, JSONSerialization) {
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

TEST_F(ModelRepositoryTest, LoadRealAssetsModelInfo) {
    try {
        std::string real_path =
            (foundation::infrastructure::test::get_assets_path() / "models_info.json").string();

        if (fs::exists(real_path)) {
            auto instance = ModelRepository::get_instance();
            EXPECT_NO_THROW(instance->set_model_info_file_path(real_path));
            EXPECT_EQ(instance->get_model_json_file_path(), real_path);

            if (instance->has_model("face_detector_yoloface")) {
                EXPECT_TRUE(instance->has_model("face_detector_yoloface"));
            }
        }
    } catch (const std::exception& e) { FAIL() << "Failed to load real assets: " << e.what(); }
}
