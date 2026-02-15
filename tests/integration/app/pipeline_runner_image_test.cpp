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
import tests.helpers.foundation.test_utilities;
import domain.face.analyser;
import domain.face;
import tests.helpers.domain.face_test_helpers;

import tests.helpers.foundation.test_constants;

using namespace services::pipeline;
using namespace tests::helpers::foundation;
using namespace domain::face::analyser;
using namespace std::chrono;

extern void LinkGlobalTestEnvironment();

class PipelineRunnerImageTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

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
        output_dir =
            std::filesystem::temp_directory_path() / "facefusion_tests" / "pipeline_runner_image";
        std::filesystem::create_directories(output_dir);
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path target_image_path_woman;
    std::filesystem::path target_image_path_babara;
    std::filesystem::path output_dir;

    // Helper to verify face swap result
    void verify_face_swap(const std::filesystem::path& output_image,
                          const std::filesystem::path& source_face,
                          float distance_threshold =
                              tests::helpers::foundation::constants::FACE_SIMILARITY_THRESHOLD) {
        ASSERT_TRUE(std::filesystem::exists(output_image))
            << "Output image does not exist: " << output_image;

        auto analyser = tests::helpers::domain::create_face_analyser(repo);

        cv::Mat output_img = cv::imread(output_image.string());
        cv::Mat source_img = cv::imread(source_face.string());

        ASSERT_FALSE(output_img.empty()) << "Failed to load output image";
        ASSERT_FALSE(source_img.empty()) << "Failed to load source image";

        auto output_faces = analyser->get_many_faces(
            output_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);
        auto source_faces = analyser->get_many_faces(
            source_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);

        ASSERT_FALSE(output_faces.empty()) << "No face detected in output image";
        ASSERT_FALSE(source_faces.empty()) << "No face detected in source image";

        // Calculate distance (smaller is more similar)
        float distance = domain::face::analyser::FaceAnalyser::calculate_face_distance(
            output_faces[0], source_faces[0]);

        EXPECT_LT(distance, distance_threshold) << "Face distance too high: " << distance
                                                << " (threshold: " << distance_threshold << ")";
    }
};

TEST_F(PipelineRunnerImageTest, ProcessSingleImage) {
    if (!std::filesystem::exists(source_path)
        || !std::filesystem::exists(target_image_path_woman)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = create_pipeline_runner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_single";
    task_config.io.source_paths.push_back(source_path.string());

    // The image to be processed (Target Media)
    task_config.io.target_paths.push_back(target_image_path_woman.string());

    task_config.io.output.path = output_dir.string();
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
    auto result = runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Image Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());

    // Expected output: output_dir/pipeline_runner_image_single_output_woman.jpg
    std::filesystem::path output_path =
        output_dir / "pipeline_runner_image_single_output_woman.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));

    verify_face_swap(output_path, source_path);
}

TEST_F(PipelineRunnerImageTest, ProcessImageBatch) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path_woman)
        || !std::filesystem::exists(target_image_path_babara)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = create_pipeline_runner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_batch";

    task_config.io.source_paths.push_back(source_path.string());

    // Add same target multiple times to simulate batch
    task_config.io.target_paths.push_back(target_image_path_woman.string());
    // Use source as target too just for variety (Lenna swapping onto Lenna)
    task_config.io.target_paths.push_back(target_image_path_babara.string());

    task_config.io.output.path = output_dir.string();
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
    auto result = runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_1 = output_dir / "pipeline_runner_image_batch_output_woman.jpg";
    std::filesystem::path output_2 = output_dir / "pipeline_runner_image_batch_output_barbara.jpg";

    EXPECT_TRUE(std::filesystem::exists(output_1));
    EXPECT_TRUE(std::filesystem::exists(output_2));

    verify_face_swap(output_1, source_path);
    verify_face_swap(output_2, source_path);
}

TEST_F(PipelineRunnerImageTest, ProcessImageSequentialMultiStep) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path_woman)
        || !std::filesystem::exists(target_image_path_babara)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = create_pipeline_runner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_multi";
    task_config.io.source_paths.push_back(source_path.string());

    task_config.io.target_paths.push_back(target_image_path_woman.string());

    task_config.io.output.path = output_dir.string();
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
    auto result = runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_path = output_dir / "pipeline_runner_image_multi_output_woman.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));

    // Resolution check (Upscaled 2x)
    cv::Mat target_img = cv::imread(target_image_path_woman.string());
    cv::Mat out_img = cv::imread(output_path.string());
    EXPECT_EQ(out_img.cols, target_img.cols * 2);
    EXPECT_EQ(out_img.rows, target_img.rows * 2);

    verify_face_swap(output_path, source_path);
}

// ============================================================================
// Performance & Stress Tests (Merged from E2E)
// ============================================================================

TEST_F(PipelineRunnerImageTest, Process720pImageCompletesWithinTimeLimit) {
    auto target_path = get_assets_path() / "standard_face_test_images" / "girl.bmp";
    if (!std::filesystem::exists(target_path)) { GTEST_SKIP() << "Test assets not found."; }

    auto output_path = output_dir / "result_girl.bmp";

    config::TaskConfig task_config;
    task_config.task_info.id = "img_720p_standard";
    task_config.io.source_paths = {source_path.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "result_";
    task_config.io.output.image_format = "bmp";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto start = steady_clock::now();
    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);

    ASSERT_TRUE(result.is_ok());

    EXPECT_LT(duration.count(), tests::helpers::foundation::constants::TIMEOUT_IMAGE_720P_MS)
        << "Processing time exceeded threshold: " << duration.count() << "ms";

    verify_face_swap(output_path, source_path);
}

TEST_F(PipelineRunnerImageTest, Process2KImageCompletesWithinTimeLimit) {
    auto target_path = get_assets_path() / "standard_face_test_images" / "woman.jpg";
    if (!std::filesystem::exists(target_path)) { GTEST_SKIP() << "Test assets not found."; }

    auto output_path = output_dir / "result_woman.png"; // WebP input, PNG output

    config::TaskConfig task_config;
    task_config.task_info.id = "img_2k_stress";
    task_config.io.source_paths = {source_path.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "result_";
    task_config.io.output.image_format = "png";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto start = steady_clock::now();
    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);

    ASSERT_TRUE(result.is_ok());

    EXPECT_LT(duration.count(), tests::helpers::foundation::constants::TIMEOUT_IMAGE_2K_MS)
        << "Processing time exceeded threshold: " << duration.count() << "ms";
}
