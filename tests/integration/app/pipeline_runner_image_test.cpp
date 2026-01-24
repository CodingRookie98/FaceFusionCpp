#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>

import services.pipeline.runner;
import config.task;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;

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
        target_image_path = get_test_data_path("standard_face_test_images/barbara.bmp");

        // Output will be generated in tests_output
        std::filesystem::create_directories("tests_output");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path target_image_path;
};

TEST_F(PipelineRunnerImageTest, ProcessSingleImage) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_single";
    task_config.io.source_paths.push_back(source_path.string());

    // The image to be processed (Target Media)
    task_config.io.target_paths.push_back(target_image_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_runner_image_single_";
    task_config.io.output.image_format = "jpg";

    // Enable Face Swapper
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Image Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());

    // Expected output: tests_output/pipeline_runner_image_single_barbara.jpg
    std::filesystem::path output_path =
        std::filesystem::path("tests_output") / "pipeline_runner_image_single_barbara.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));

    // Verify it is a valid image
    cv::Mat img = cv::imread(output_path.string());
    EXPECT_FALSE(img.empty());
    EXPECT_GT(img.cols, 0);
    EXPECT_GT(img.rows, 0);
}

TEST_F(PipelineRunnerImageTest, ProcessImageBatch) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_batch";

    task_config.io.source_paths.push_back(source_path.string());

    // Add same target multiple times to simulate batch
    task_config.io.target_paths.push_back(target_image_path.string());
    // Use source as target too just for variety (Lenna swapping onto Lenna)
    task_config.io.target_paths.push_back(source_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "batch_";
    task_config.io.output.image_format = "jpg";

    task_config.resource.execution_order = config::ExecutionOrder::Batch;

    // Enable Face Enhancer
    config::PipelineStep step1;
    step1.step = "face_enhancer";
    step1.enabled = true;
    config::FaceEnhancerParams params1;
    params1.model = "gfpgan_1.4";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_1 = std::filesystem::path("tests_output") / "batch_barbara.jpg";
    std::filesystem::path output_2 = std::filesystem::path("tests_output") / "batch_lenna.jpg";

    EXPECT_TRUE(std::filesystem::exists(output_1));
    EXPECT_TRUE(std::filesystem::exists(output_2));
}

TEST_F(PipelineRunnerImageTest, ProcessImageSequentialMultiStep) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_image_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_image_multi";
    task_config.io.source_paths.push_back(source_path.string());

    task_config.io.target_paths.push_back(target_image_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "multi_";
    task_config.io.output.image_format = "jpg";

    // Swapper + Enhancer
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    config::PipelineStep step2;
    step2.step = "face_enhancer";
    step2.enabled = true;
    config::FaceEnhancerParams params2;
    params2.model = "gfpgan_1.4";
    step2.params = params2;
    task_config.pipeline.push_back(step2);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    ASSERT_TRUE(result.is_ok());

    std::filesystem::path output_path = std::filesystem::path("tests_output") / "multi_barbara.jpg";
    EXPECT_TRUE(std::filesystem::exists(output_path));
}
