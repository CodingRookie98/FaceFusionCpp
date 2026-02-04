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
    auto result = Result<int>::ok(42);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    EXPECT_EQ(result.value(), 42);
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(ResultTest, ErrValue) {
    auto result = Result<int>::err(ConfigError(ErrorCode::E200ConfigError, "test error", "field"));
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "test error");
    EXPECT_EQ(result.error().yaml_path, "field");
    EXPECT_FALSE(static_cast<bool>(result));
}

TEST(ResultTest, VoidSpecialization) {
    auto result = Result<void>::ok();
    EXPECT_TRUE(result.is_ok());

    auto err = Result<void>::err(ConfigError(ErrorCode::E200ConfigError, "void error"));
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

    auto result = validate_app_config(config);
    EXPECT_TRUE(result.is_ok()) << (result.is_err() ? result.error().formatted() : "");
}

TEST(AppConfigValidationTest, EmptyModelsPath) {
    AppConfig config;
    config.config_version = "1.0";
    config.models.path = "non_existent_path_xyz";
    config.logging.directory = "./logs";

    auto result = validate_app_config(config);
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
    config.io.output.image_format = "png";
    config.io.output.video_encoder = "libx264";
    config.io.output.video_quality = 80;
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_ok()) << (result.is_err() ? result.error().formatted() : "");
}

TEST(TaskConfigValidationTest, InvalidVideoQuality) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.io.output.image_format = "png";
    config.io.output.video_quality = 150; // Invalid
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().yaml_path, Eq("io.output.video_quality"));
}

// ============================================================================
// ConfigValidator Enhancement Tests (Task M9)
// ============================================================================

TEST(TaskConfigValidationTest, VersionMismatch) {
    TaskConfig config;
    config.config_version = "2.0"; // Unsupported
    config.task_info.id = "test";

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E204ConfigVersionMismatch);
    EXPECT_EQ(result.error().yaml_path, "config_version");
}

TEST(TaskConfigValidationTest, FaceAnalysisRangeValidation) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.io.output.image_format = "png";

    // Invalid detector threshold
    config.face_analysis.face_detector.score_threshold = 1.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E202ParameterOutOfRange);
    EXPECT_EQ(result.error().yaml_path, "face_analysis.face_detector.score_threshold");

    // Invalid similarity threshold
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = -0.1;

    result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E202ParameterOutOfRange);
    EXPECT_EQ(result.error().yaml_path, "face_analysis.face_recognizer.similarity_threshold");
}

TEST(TaskConfigValidationTest, ReferenceFacePathRequired) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.io.output.image_format = "png";

    PipelineStep step;
    step.step = "face_swapper";
    FaceSwapperParams params;
    params.face_selector_mode = FaceSelectorMode::Reference;
    params.reference_face_path = std::nullopt; // Missing
    step.params = params;
    config.pipeline.push_back(step);

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E205RequiredFieldMissing);
    EXPECT_EQ(result.error().yaml_path, "pipeline[0].params.reference_face_path");

    // Test empty path
    params.reference_face_path = "";
    config.pipeline[0].params = params;
    result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E205RequiredFieldMissing);
}

TEST(TaskConfigValidationTest, ReferenceFacePathExists) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.source_paths = {"."};
    config.io.target_paths = {"."};
    config.io.output.path = ".";
    config.io.output.image_format = "png";

    PipelineStep step;
    step.step = "face_swapper";
    FaceSwapperParams params;
    params.face_selector_mode = FaceSelectorMode::Reference;
    params.reference_face_path = "non_existent_path_xyz.jpg";
    step.params = params;
    config.pipeline.push_back(step);

    auto result = validate_task_config(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().code, ErrorCode::E206InvalidPath);
    EXPECT_EQ(result.error().yaml_path, "pipeline[0].params.reference_face_path");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
