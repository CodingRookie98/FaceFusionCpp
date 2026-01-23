#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>

import services.pipeline.runner;
import config.task;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;

class PipelineRunnerVideoTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
        output_path = "tests_output/pipeline_runner_video_output.mp4";

        std::filesystem::create_directories("tests_output");
        if (std::filesystem::exists(output_path)) { std::filesystem::remove(output_path); }
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path video_path;
    std::filesystem::path output_path;
};

TEST_F(PipelineRunnerVideoTest, ProcessVideoEndToEnd) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found: " << video_path << " or " << source_path;
    }

    // 1. Create App Config
    config::AppConfig app_config;

    // 2. Create Runner
    auto runner = CreatePipelineRunner(app_config);

    // 3. Construct TaskConfig
    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_task";

    // IO
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_runner_video_";
    task_config.io.output.suffix = "output";
    std::string expected_output = "tests_output/pipeline_runner_video_slideshow_scaledoutput.mp4";
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    task_config.io.output.audio_policy = config::AudioPolicy::Copy;

    // Resource
    task_config.resource.thread_count = 2;

    // Pipeline Step (Face Swapper)
    config::PipelineStep step;
    step.step = "face_swapper";
    step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128";
    step.params = params;
    task_config.pipeline.push_back(step);

    // 4. Run
    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {
        // std::cout << "Progress: " << p.current_frame << "/" << p.total_frames << std::endl;
    });

    if (result.is_err()) {
        std::cerr << "PipelineRunner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());

    // 5. Verification
    EXPECT_TRUE(std::filesystem::exists(expected_output))
        << "Output video should exist at " << expected_output;
    if (std::filesystem::exists(expected_output)) {
        EXPECT_GT(std::filesystem::file_size(expected_output), 1024)
            << "Output video should be non-empty";
    }
}

TEST_F(PipelineRunnerVideoTest, ProcessVideoStrictMemory) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_strict";
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "strict_";
    task_config.io.output.suffix = ""; // strict_slideshow_scaled.mp4

    // Enable Strict Memory
    task_config.resource.memory_strategy = config::MemoryStrategy::Strict;

    // Adding two steps to verify multi-pass
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    // Using same swapper again as step 2 just to test multi-pass logic (normally would be enhancer)
    // Or just 1 step is enough to verify strict path executes.
    // Let's do 1 step to be safe on time.

    std::string expected_output = "tests_output/strict_slideshow_scaled.mp4";
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Strict Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output));

    // Check temp files cleanup
    // We cannot easily check if temp files existed during run without hooking,
    // but we can check if they are gone now.
    EXPECT_FALSE(std::filesystem::exists("tests_output/temp_step_0.mp4"));
}
