/**
 * @file config_validator_test.cpp
 * @brief Unit tests for ConfigValidator
 * @date 2026-02-05
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <filesystem>

import config.validator;
import config.task;
import config.types;
import config.app;

using namespace config;

// Test fixture for ConfigValidator
class ConfigValidatorTest : public ::testing::Test {
protected:
    ConfigValidator validator;
    TaskConfig valid_task_config;

    void SetUp() override {
        // Setup a valid task config as a baseline
        valid_task_config.task_info.id = "test_task_01";
        valid_task_config.task_info.description = "Test Description";
        
        // Use current file as a valid existing path for source/target
        std::string current_file = __FILE__;
        
        valid_task_config.io.source_paths.push_back(current_file);
        valid_task_config.io.target_paths.push_back(current_file);
        
        valid_task_config.io.output.path = "/tmp"; // Assuming /tmp exists on Linux
        valid_task_config.io.output.image_format = "jpg";
        valid_task_config.io.output.video_quality = 18;

        valid_task_config.face_analysis.face_detector.models = {"yoloface"};
        valid_task_config.face_analysis.face_detector.score_threshold = 0.5f;
        valid_task_config.face_analysis.face_recognizer.similarity_threshold = 0.6f;

        // Add a valid pipeline step
        PipelineStep step;
        step.step = "face_swapper";
        step.name = "swapper_1";
        FaceSwapperParams params;
        params.model = "inswapper_128";
        params.face_selector_mode = FaceSelectorMode::Many;
        step.params = params;
        valid_task_config.pipeline.push_back(step);
    }
};

TEST_F(ConfigValidatorTest, ValidateValidConfigReturnsEmptyErrors) {
    auto errors = validator.validate(valid_task_config);
    if (!errors.empty()) {
        for (const auto& err : errors) {
            std::cout << "Unexpected error: " << err.yaml_path << " - " << err.expected << std::endl;
        }
    }
    EXPECT_TRUE(errors.empty());
}

TEST_F(ConfigValidatorTest, ValidateInvalidTaskIdReturnsError) {
    valid_task_config.task_info.id = "invalid-task-id!"; // Contains special chars
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "task_info.id");
}

TEST_F(ConfigValidatorTest, ValidateEmptySourcePathsReturnsError) {
    valid_task_config.io.source_paths.clear();
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    bool found = false;
    for (const auto& err : errors) {
        if (err.yaml_path == "io.source_paths") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigValidatorTest, ValidateNonExistentSourcePathReturnsError) {
    valid_task_config.io.source_paths.push_back("/non/existent/path.jpg");
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    // Error could be in any order if multiple checks fail, but here we expect at least one
    bool found = false;
    for (const auto& err : errors) {
        if (err.yaml_path.find("io.source_paths") != std::string::npos && err.code == ErrorCode::E206InvalidPath) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(ConfigValidatorTest, ValidateInvalidDetectorScoreReturnsError) {
    valid_task_config.face_analysis.face_detector.score_threshold = 1.5f;
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "face_analysis.face_detector.score_threshold");
}

TEST_F(ConfigValidatorTest, ValidateInvalidRecognizerThresholdReturnsError) {
    valid_task_config.face_analysis.face_recognizer.similarity_threshold = -0.1f;
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "face_analysis.face_recognizer.similarity_threshold");
}

TEST_F(ConfigValidatorTest, ValidateInvalidImageFormatReturnsError) {
    valid_task_config.io.output.image_format = "gif"; // Unsupported
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "io.output.image_format");
}

TEST_F(ConfigValidatorTest, ValidateEmptyPipelineReturnsError) {
    valid_task_config.pipeline.clear();
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "pipeline");
}

TEST_F(ConfigValidatorTest, ValidateInvalidPipelineStepTypeReturnsError) {
    valid_task_config.pipeline[0].step = "invalid_step";
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "pipeline[0].step");
}

TEST_F(ConfigValidatorTest, ValidateReferenceModeMissingPathReturnsError) {
    auto& params = std::get<FaceSwapperParams>(valid_task_config.pipeline[0].params);
    params.face_selector_mode = FaceSelectorMode::Reference;
    params.reference_face_path = std::nullopt;
    
    auto errors = validator.validate(valid_task_config);
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors[0].yaml_path, "pipeline[0].params.reference_face_path");
}

TEST_F(ConfigValidatorTest, ValidateOrErrorReturnsOkForValidConfig) {
    auto result = validator.validate_or_error(valid_task_config);
    EXPECT_TRUE(result.is_ok());
}

TEST_F(ConfigValidatorTest, ValidateOrErrorReturnsErrForInvalidConfig) {
    valid_task_config.io.output.video_quality = 200;
    auto result = validator.validate_or_error(valid_task_config);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().yaml_path, "io.output.video_quality");
}
