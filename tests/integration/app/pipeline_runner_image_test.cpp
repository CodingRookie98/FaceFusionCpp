/**
 * @file pipeline_runner_image_test.cpp
 * @brief Integration tests for image processing with PipelineRunner
 * @author CodingRookie
 * @date 2026-01-27
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

import services.pipeline.runner;
import config.task;
import config.merger;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;
import domain.face.analyser;
import domain.face;
import domain.face.test_support;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;
using namespace domain::face::analyser;

class PipelineRunnerImageTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        target_image_path_woman = get_test_data_path("standard_face_test_images/woman.jpg");
        target_image_path_babara = get_test_data_path("standard_face_test_images/barbara.bmp");

        // Output will be generated in tests_output
        std::filesystem::create_directories("tests_output");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path target_image_path_woman;
    std::filesystem::path target_image_path_babara;
};

TEST_F(PipelineRunnerImageTest, ProcessSingleImage) {
    if (!std::filesystem::exists(source_path)
        || !std::filesystem::exists(target_image_path_woman)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_single";
    task_config.io.source_paths.push_back(source_path.string());

    // The image to be processed (Target Media)
    task_config.io.target_paths.push_back(target_image_path_woman.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_runner_image_single_output_";
    task_config.io.output.image_format = "jpg";

    // Enable Face Swapper
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Image Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());

    // Expected output: tests_output/pipeline_runner_image_single_output_woman.jpg
    std::filesystem::path output_path =
        std::filesystem::path("tests_output") / "pipeline_runner_image_single_output_woman.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));

    // Verify it is a valid image
    cv::Mat img = cv::imread(output_path.string());
    EXPECT_FALSE(img.empty());
    EXPECT_GT(img.cols, 0);
    EXPECT_GT(img.rows, 0);

    // Resolution check
    cv::Mat source_img = cv::imread(source_path.string());
    cv::Mat target_img = cv::imread(target_image_path_woman.string());
    EXPECT_EQ(img.cols, target_img.cols);
    EXPECT_EQ(img.rows, target_img.rows);

    // Similarity check
    auto analyser = domain::face::test_support::create_face_analyser(repo);
    auto source_faces = analyser->get_many_faces(
        source_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    auto output_faces =
        analyser->get_many_faces(img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    if (!source_faces.empty() && !output_faces.empty()) {
        float distance = FaceAnalyser::calculate_face_distance(source_faces[0], output_faces[0]);
        // Expect similarity (distance < 0.6 is a common threshold for ArcFace)
        // Since we swapped Lenna onto Woman, the result face should look like Lenna.
        EXPECT_LT(distance, 0.65f) << "Swapped face should resemble source face";
    } else {
        FAIL()
            << "Face detection failed for similarity check in SingleImage test. Output image might be corrupted or face not found.";
    }
}

TEST_F(PipelineRunnerImageTest, ProcessImageBatch) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path_woman)
        || !std::filesystem::exists(target_image_path_babara)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_batch";

    task_config.io.source_paths.push_back(source_path.string());

    // Add same target multiple times to simulate batch
    task_config.io.target_paths.push_back(target_image_path_woman.string());
    // Use source as target too just for variety (Lenna swapping onto Lenna)
    task_config.io.target_paths.push_back(target_image_path_babara.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_runner_image_batch_output_";
    task_config.io.output.image_format = "jpg";

    task_config.resource.execution_order = config::ExecutionOrder::Batch;

    // Enable Face Swapper
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_1 =
        std::filesystem::path("tests_output") / "pipeline_runner_image_batch_output_woman.jpg";
    std::filesystem::path output_2 =
        std::filesystem::path("tests_output") / "pipeline_runner_image_batch_output_barbara.jpg";

    EXPECT_TRUE(std::filesystem::exists(output_1));
    EXPECT_TRUE(std::filesystem::exists(output_2));

    // Similarity check for output_1 (Lenna -> Woman)
    auto analyser = domain::face::test_support::create_face_analyser(repo);
    cv::Mat source_img = cv::imread(source_path.string());
    auto source_faces = analyser->get_many_faces(
        source_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    if (!source_faces.empty()) {
        cv::Mat out1_img = cv::imread(output_1.string());
        auto out1_faces = analyser->get_many_faces(
            out1_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
        if (!out1_faces.empty()) {
            float distance1 = FaceAnalyser::calculate_face_distance(source_faces[0], out1_faces[0]);
            EXPECT_LT(distance1, 0.65f) << "Batch Output 1 should resemble source face";
        }

        // Similarity check for output_2 (Lenna -> Barbara)
        cv::Mat out2_img = cv::imread(output_2.string());
        auto out2_faces = analyser->get_many_faces(
            out2_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
        if (!out2_faces.empty()) {
            float distance2 = FaceAnalyser::calculate_face_distance(source_faces[0], out2_faces[0]);
            EXPECT_LT(distance2, 0.65f) << "Batch Output 2 should resemble source face";
        }
    }
}

TEST_F(PipelineRunnerImageTest, ProcessImageSequentialMultiStep) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path_woman)
        || !std::filesystem::exists(target_image_path_babara)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_multi";
    task_config.io.source_paths.push_back(source_path.string());

    task_config.io.target_paths.push_back(target_image_path_woman.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_runner_image_multi_output_";
    task_config.io.output.image_format = "jpg";

    // Swapper -> ExpressionRestorer -> Enhancer -> FrameUpscaler
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    config::PipelineStep step2;
    step2.step = "expression_restorer";
    step2.enabled = true;
    config::ExpressionRestorerParams params2;
    params2.model = "live_portrait";
    step2.params = params2;
    task_config.pipeline.push_back(step2);

    config::PipelineStep step3;
    step3.step = "face_enhancer";
    step3.enabled = true;
    config::FaceEnhancerParams params3;
    params3.model = "gfpgan_1.4";
    step3.params = params3;
    task_config.pipeline.push_back(step3);

    config::PipelineStep step4;
    step4.step = "frame_enhancer";
    step4.enabled = true;
    config::FrameEnhancerParams params4;
    params4.model = "real_esrgan_x2_fp16";
    step4.params = params4;
    task_config.pipeline.push_back(step4);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_path =
        std::filesystem::path("tests_output") / "pipeline_runner_image_multi_output_woman.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));

    // Resolution check (Upscaled 2x)
    cv::Mat target_img = cv::imread(target_image_path_woman.string());
    cv::Mat out_img = cv::imread(output_path.string());
    EXPECT_EQ(out_img.cols, target_img.cols * 2);
    EXPECT_EQ(out_img.rows, target_img.rows * 2);

    // Similarity check
    auto analyser = domain::face::test_support::create_face_analyser(repo);
    cv::Mat source_img = cv::imread(source_path.string());
    auto source_faces = analyser->get_many_faces(
        source_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

    if (!source_faces.empty()) {
        // We need to find face in upscaled image
        auto out_faces = analyser->get_many_faces(
            out_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
        if (!out_faces.empty()) {
            float distance = FaceAnalyser::calculate_face_distance(source_faces[0], out_faces[0]);
            EXPECT_LT(distance, 0.65f) << "Multi-step output should resemble source face";
        }
    }
}
