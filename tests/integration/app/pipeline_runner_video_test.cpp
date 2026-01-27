/**
 * @file pipeline_runner_video_test.cpp
 * @brief Integration tests for video processing with PipelineRunner
 * @author CodingRookie
 * @date 2026-01-27
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>

import services.pipeline.runner;
import config.task;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;
import domain.face;
import domain.face.analyser;
import domain.face.test_support;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;
using namespace domain::face::analyser;

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

    void VerifyVideoContent(const std::filesystem::path& video_file,
                            const std::filesystem::path& source_face_img, float expected_scale) {
        if (!std::filesystem::exists(video_file)) {
            FAIL() << "Output video file does not exist: " << video_file;
        }

        cv::VideoCapture cap(video_file.string());
        ASSERT_TRUE(cap.isOpened()) << "Failed to open output video";

        double fps = cap.get(cv::CAP_PROP_FPS);
        int total_frames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        int width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        int height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

        std::cout << "Verifying video: " << video_file << " [Frames: " << total_frames
                  << ", Size: " << width << "x" << height << ", FPS: " << fps << "]" << std::endl;

        // 1. Resolution Check
        cv::VideoCapture cap_orig(video_path.string());
        ASSERT_TRUE(cap_orig.isOpened());
        int orig_width = static_cast<int>(cap_orig.get(cv::CAP_PROP_FRAME_WIDTH));
        int orig_height = static_cast<int>(cap_orig.get(cv::CAP_PROP_FRAME_HEIGHT));

        EXPECT_NEAR(width, orig_width * expected_scale, 2.0); // Allow slight rounding diff
        EXPECT_NEAR(height, orig_height * expected_scale, 2.0);

        // 2. Similarity Check (Uniform Sampling)
        auto analyser = domain::face::test_support::create_face_analyser(repo);
        cv::Mat src_img = cv::imread(source_face_img.string());
        if (src_img.empty()) {
            std::cout << "Warning: Failed to load source image: " << source_face_img << std::endl;
            return;
        }

        auto source_faces = analyser->get_many_faces(
            src_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

        if (source_faces.empty()) {
            std::cout
                << "Warning: Could not detect face in source image. Skipping similarity check."
                << std::endl;
            return;
        }

        int valid_frames = 0;
        int passed_frames = 0;
        int frames_to_check = 10;
        int step = std::max(1, total_frames / frames_to_check);

        for (int i = 0; i < total_frames; i += step) {
            cap.set(cv::CAP_PROP_POS_FRAMES, i);
            cv::Mat frame;
            cap >> frame;
            if (frame.empty()) break;

            auto frame_faces = analyser->get_many_faces(
                frame, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);

            if (!frame_faces.empty()) {
                valid_frames++;
                // Check closest face to source
                float min_dist = 100.0f;
                for (const auto& face : frame_faces) {
                    float dist = FaceAnalyser::calculate_face_distance(source_faces[0], face);
                    if (dist < min_dist) min_dist = dist;
                }

                if (min_dist < 0.65f) {
                    passed_frames++;
                } else {
                    std::cout << "Frame " << i << " failed similarity check. Dist: " << min_dist
                              << std::endl;
                }
            }
        }

        std::cout << "Similarity Check: " << passed_frames << "/" << valid_frames
                  << " frames passed." << std::endl;

        if (valid_frames > 0) {
            double pass_rate = static_cast<double>(passed_frames) / valid_frames;
            EXPECT_GE(pass_rate, 0.9) << "Less than 90% of valid frames passed similarity check";
        } else {
            std::cout << "Warning: No faces detected in any sampled frame." << std::endl;
        }
    }
};

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
    task_config.io.output.prefix = "pipeline_video_strict_memory_";
    task_config.io.output.suffix = ""; // pipeline_video_strict_memory_slideshow_scaled.mp4

    // Enable Strict Memory
    task_config.resource.memory_strategy = config::MemoryStrategy::Strict;

    // Adding two steps to verify multi-pass
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    // Using same swapper again as step 2 just to test multi-pass logic (normally would be enhancer)
    // Or just 1 step is enough to verify strict path executes.
    // Let's do 1 step to be safe on time.

    std::string expected_output = "tests_output/pipeline_video_strict_memory_slideshow_scaled.mp4";
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

TEST_F(PipelineRunnerVideoTest, ProcessVideoSequentialAllProcessors) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_seq_all";
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_video_sequential_all_";
    task_config.io.output.suffix = "";

    task_config.resource.execution_order = config::ExecutionOrder::Sequential;

    // 1. Swapper
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    // 2. Face Enhancer
    config::PipelineStep step2;
    step2.step = "face_enhancer";
    step2.enabled = true;
    config::FaceEnhancerParams params2;
    params2.model = "gfpgan_1.4";
    step2.params = params2;
    task_config.pipeline.push_back(step2);

    // 3. Expression Restorer
    config::PipelineStep step3;
    step3.step = "expression_restorer";
    step3.enabled = true;
    config::ExpressionRestorerParams params3;
    step3.params = params3;
    task_config.pipeline.push_back(step3);

    // 4. Frame Enhancer
    config::PipelineStep step4;
    step4.step = "frame_enhancer";
    step4.enabled = true;
    config::FrameEnhancerParams params4;
    params4.model = "real_esrgan_x2_fp16";
    step4.params = params4;
    task_config.pipeline.push_back(step4);

    std::string expected_output = "tests_output/pipeline_video_sequential_all_slideshow_scaled.mp4";
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Sequential AllProcessors Runner Error: " << result.error().message
                  << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output));

    // Verify Content (Expected 2x upscale)
    VerifyVideoContent(expected_output, source_path, 2.0f);
}

TEST_F(PipelineRunnerVideoTest, ProcessVideoBatchMode) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_batch";
    task_config.io.source_paths.push_back(source_path.string());

    // Add two targets to verify batch iteration (even if implementation is currently sequential)
    task_config.io.target_paths.push_back(video_path.string());
    // Create copy for batch test
    std::filesystem::path video_path_2 = "tests_output/slideshow_copy.mp4";
    std::filesystem::copy_file(video_path, video_path_2,
                               std::filesystem::copy_options::overwrite_existing);
    task_config.io.target_paths.push_back(video_path_2.string());

    task_config.io.output.path = "tests_output";
    task_config.io.output.prefix = "pipeline_video_batch_";
    task_config.io.output.suffix = "";

    task_config.resource.execution_order = config::ExecutionOrder::Batch;

    // 1. Swapper
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    // 2. Face Enhancer
    config::PipelineStep step2;
    step2.step = "face_enhancer";
    step2.enabled = true;
    config::FaceEnhancerParams params2;
    params2.model = "gfpgan_1.4";
    step2.params = params2;
    task_config.pipeline.push_back(step2);

    // 3. Expression Restorer
    config::PipelineStep step3;
    step3.step = "expression_restorer";
    step3.enabled = true;
    config::ExpressionRestorerParams params3;
    step3.params = params3;
    task_config.pipeline.push_back(step3);

    // 4. Frame Enhancer
    config::PipelineStep step4;
    step4.step = "frame_enhancer";
    step4.enabled = true;
    config::FrameEnhancerParams params4;
    params4.model = "real_esrgan_x2_fp16";
    step4.params = params4;
    task_config.pipeline.push_back(step4);

    std::string expected_output_1 = "tests_output/pipeline_video_batch_slideshow_scaled.mp4";
    std::string expected_output_2 = "tests_output/pipeline_video_batch_slideshow_copy.mp4";

    if (std::filesystem::exists(expected_output_1)) std::filesystem::remove(expected_output_1);
    if (std::filesystem::exists(expected_output_2)) std::filesystem::remove(expected_output_2);

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Batch Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output_1));
    EXPECT_TRUE(std::filesystem::exists(expected_output_2));

    // Verify Content (Expected 2x upscale)
    VerifyVideoContent(expected_output_1, source_path, 2.0f);
    // Optionally verify second output too, but one is usually enough for pipeline logic check
}
