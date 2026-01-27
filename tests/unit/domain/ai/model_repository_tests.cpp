
/**
 * @file model_repository_tests.cpp
 * @brief Unit tests for ModelRepository.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

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

        // Reset singleton state
        auto instance = ModelRepository::get_instance();
        instance->set_base_path("");
        instance->set_download_strategy(DownloadStrategy::Auto);
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

TEST_F(ModelRepositoryTest, JSONSerializationFileName) {
    nlohmann::json j = {
        {"name", "test"}, {"type", "type"}, {"file_name", "test.onnx"}, {"url", "url"}};

    ModelInfo deserialized;
    from_json(j, deserialized);

    EXPECT_EQ(deserialized.name, "test");
    EXPECT_EQ(deserialized.path, "test.onnx");
}

TEST_F(ModelRepositoryTest, SetBasePath) {
    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(test_json_path);

    // Default behavior
    EXPECT_EQ(instance->get_model_path("test_model_1"), "./models/test_model_1.onnx");

    // Set base path
    std::string base = "custom/path";
    instance->set_base_path(base);

    // Expect joined path
    std::filesystem::path expected = std::filesystem::path(base) / "test_model_1.onnx";
    EXPECT_EQ(instance->get_model_path("test_model_1"), expected.string());
}

TEST_F(ModelRepositoryTest, DownloadStrategySkip) {
    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(test_json_path);
    instance->set_download_strategy(DownloadStrategy::Skip);

    // model doesn't exist locally
    std::string path = instance->ensure_model("test_model_1");
    EXPECT_TRUE(path.empty());
}

TEST_F(ModelRepositoryTest, FileNameSupport) {
    std::string json_path = "test_filename.json";
    std::ofstream file(json_path);
    file << R"({
        "models_info": [
            {
                "name": "new_model",
                "type": "face_enhancer",
                "url": "http://example.com/new.onnx",
                "file_name": "new_model.onnx"
            }
        ]
    })";
    file.close();

    auto instance = ModelRepository::get_instance();
    instance->set_model_info_file_path(json_path);

    // Set base path to verify joining
    instance->set_base_path("./assets/models");

    auto info = instance->get_model_info("new_model");
    std::filesystem::path expected = std::filesystem::path("./assets/models") / "new_model.onnx";
    EXPECT_EQ(info.path, expected.string());

    if (fs::exists(json_path)) { fs::remove(json_path); }
}

TEST_F(ModelRepositoryTest, LoadRealAssetsModelInfo) {
    try {
        std::string real_path =
            (foundation::infrastructure::test::get_assets_path() / "models_info.json").string();

        if (fs::exists(real_path)) {
            auto instance = ModelRepository::get_instance();
            EXPECT_NO_THROW(instance->set_model_info_file_path(real_path));
            EXPECT_EQ(instance->get_model_json_file_path(), real_path);

            if (instance->has_model("yoloface")) {
                auto info = instance->get_model_info("yoloface");
                EXPECT_FALSE(info.path.empty());
            }
        }
    } catch (const std::exception& e) { FAIL() << "Failed to load real assets: " << e.what(); }
}
