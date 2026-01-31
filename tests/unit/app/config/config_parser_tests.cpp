#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

import config.parser;
import config.types;

using namespace config;
using namespace testing;

// ============================================================================
// Result<T, E> 类型测试
// ============================================================================

TEST(ResultTest, OkValue) {
    auto result = Result<int>::Ok(42);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    EXPECT_EQ(result.value(), 42);
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(ResultTest, ErrValue) {
    auto result = Result<int>::Err(ConfigError(ErrorCode::E200_ConfigError, "test error", "field"));
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "test error");
    EXPECT_EQ(result.error().yaml_path, "field");
    EXPECT_FALSE(static_cast<bool>(result));
}

TEST(ResultTest, VoidSpecialization) {
    auto result = Result<void>::Ok();
    EXPECT_TRUE(result.is_ok());

    auto err = Result<void>::Err(ConfigError(ErrorCode::E200_ConfigError, "void error"));
    EXPECT_TRUE(err.is_err());
    EXPECT_EQ(err.error().message, "void error");
}

// ============================================================================
// AppConfig 校验测试
// ============================================================================

TEST(AppConfigValidationTest, ValidConfig) {
    AppConfig config;
    config.config_version = "1.0";
    config.models.path = "."; // Use current dir which exists
    config.logging.directory = "./logs";

    auto result = ValidateAppConfig(config);
    EXPECT_TRUE(result.is_ok()) << (result.is_err() ? result.error().formatted() : "");
}

TEST(AppConfigValidationTest, EmptyModelsPath) {
    AppConfig config;
    config.config_version = "1.0";
    config.models.path = "non_existent_path_xyz";
    config.logging.directory = "./logs";

    auto result = ValidateAppConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().yaml_path, Eq("models.path"));
}

// ============================================================================
// TaskConfig 校验测试
// ============================================================================

TEST(TaskConfigValidationTest, ValidConfig) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test_task";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_ok()) << (result.is_err() ? result.error().formatted() : "");
}

TEST(TaskConfigValidationTest, InvalidVideoQuality) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.io.output.video_quality = 150; // Invalid
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().yaml_path, Eq("io.output.video_quality"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
