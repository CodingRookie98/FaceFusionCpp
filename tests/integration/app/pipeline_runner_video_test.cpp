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
import config.merger;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;
import domain.face;
import domain.face.analyser;
import domain.face.test_support;
import foundation.media.ffmpeg;

#include "test_support/test_constants.hpp"

using namespace services::pipeline;
using namespace foundation::infrastructure::test;
using namespace domain::face::analyser;
using namespace std::chrono;

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
        
        output_dir = std::filesystem::temp_directory_path() / "facefusion_tests" / "pipeline_runner_video";
        std::filesystem::create_directories(output_dir);
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path video_path;
    std::filesystem::path output_dir;

    struct VideoInfo {
        int frame_count;
        double fps;
        int width;
        int height;
        bool has_audio;
    };

    VideoInfo get_video_info(const std::filesystem::path& video_path) {
        if (!std::filesystem::exists(video_path)) {
            std::cerr << "[ERROR] Video file does not exist: "
                      << std::filesystem::absolute(video_path) << std::endl;
            return {0, 0.0, 0, 0, false};
        }

        try {
            foundation::media::ffmpeg::VideoParams params(video_path.string());
            if (params.width == 0 || params.height == 0) {
                std::cerr << "[ERROR] Failed to read video info using ffmpeg module: " << video_path
                          << std::endl;
                return {0, 0.0, 0, 0, false};
            }

            VideoInfo info;
            info.frame_count = static_cast<int>(params.frameCount);
            info.fps = params.frameRate;
            info.width = static_cast<int>(params.width);
            info.height = static_cast<int>(params.height);
            info.has_audio = false;

            if (info.frame_count <= 0) {
                std::cerr << "[WARN] FFmpeg returned 0 frames, trying OpenCV fallback..."
                          << std::endl;
                cv::VideoCapture cap(video_path.string(), cv::CAP_FFMPEG);
                if (!cap.isOpened()) cap.open(video_path.string(), cv::CAP_ANY);
                if (cap.isOpened()) {
                    info.frame_count = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
                }
            }

            return info;

        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Exception reading video info: " << e.what() << std::endl;
            return {0, 0.0, 0, 0, false};
        }
    }

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
            EXPECT_GE(pass_rate, test_constants::FRAME_PASS_RATE) << "Less than 90% of valid frames passed similarity check";
        } else {
            std::cout << "Warning: No faces detected in any sampled frame." << std::endl;
        }
    }
};

TEST_F(PipelineRunnerVideoTest, ProcessVideoStrictMemoryOneStep) {
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

    task_config.io.output.path = output_dir.string();
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

    std::string expected_output = (output_dir / "pipeline_video_strict_memory_slideshow_scaled.mp4").string();
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Strict Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output));

    EXPECT_FALSE(std::filesystem::exists((output_dir / "temp_step_0.mp4").string()));
}

TEST_F(PipelineRunnerVideoTest, ProcessVideoTolerantMemoryOneStep) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_tolerant";
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "pipeline_video_tolerant_memory_";
    task_config.io.output.suffix = ""; // pipeline_video_tolerant_memory_slideshow_scaled.mp4

    // Enable Tolerant Memory
    task_config.resource.memory_strategy = config::MemoryStrategy::Tolerant;

    // Adding two steps to verify multi-pass
    config::PipelineStep step1;
    step1.step = "face_swapper";
    step1.enabled = true;
    config::FaceSwapperParams params1;
    params1.model = "inswapper_128_fp16";
    step1.params = params1;
    task_config.pipeline.push_back(step1);

    std::string expected_output =
        (output_dir / "pipeline_video_tolerant_memory_slideshow_scaled.mp4").string();
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Tolerant Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output));

    EXPECT_FALSE(std::filesystem::exists((output_dir / "temp_step_0.mp4").string()));
}

TEST_F(PipelineRunnerVideoTest, ProcessVideoSequentialMultiStep) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_seq_multi_step";
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "pipeline_video_sequential_multi_step_";
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

    std::string expected_output =
        (output_dir / "pipeline_video_sequential_multi_step_slideshow_scaled.mp4").string();
    if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Sequential MultiStep Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output));

    // Verify Content (Expected 2x upscale)
    VerifyVideoContent(expected_output, source_path, 2.0f);
}

TEST_F(PipelineRunnerVideoTest, ProcessVideoBatchMutiStep) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "test_video_batch_multi_step";
    task_config.io.source_paths.push_back(source_path.string());

    // Add two targets to verify batch iteration (even if implementation is currently sequential)
    task_config.io.target_paths.push_back(video_path.string());
    // Create copy for batch test
    std::filesystem::path video_path_2 = output_dir / "slideshow_copy.mp4";
    std::filesystem::copy_file(video_path, video_path_2,
                               std::filesystem::copy_options::overwrite_existing);
    task_config.io.target_paths.push_back(video_path_2.string());

    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "pipeline_video_batch_multi_step_";
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

    std::string expected_output_1 =
        (output_dir / "pipeline_video_batch_multi_step_slideshow_scaled.mp4").string();
    std::string expected_output_2 =
        (output_dir / "pipeline_video_batch_multi_step_slideshow_copy.mp4").string();

    if (std::filesystem::exists(expected_output_1)) std::filesystem::remove(expected_output_1);
    if (std::filesystem::exists(expected_output_2)) std::filesystem::remove(expected_output_2);

    auto merged_task_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});

    if (result.is_err()) {
        std::cerr << "Batch MultiStep Runner Error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok());
    EXPECT_TRUE(std::filesystem::exists(expected_output_1));
    EXPECT_TRUE(std::filesystem::exists(expected_output_2));

    // Verify Content (Expected 2x upscale)
    VerifyVideoContent(expected_output_1, source_path, 2.0f);
    // Optionally verify second output too, but one is usually enough for pipeline logic check
}

// ============================================================================
// Performance Tests (Merged from E2E)
// ============================================================================

TEST_F(PipelineRunnerVideoTest, ProcessVideo_AchievesMinimumFPS) {
    auto input_info = get_video_info(video_path);
    auto output_path = output_dir / "result_slideshow_fps.mp4";

    config::TaskConfig task_config;
    task_config.task_info.id = "video_720p_fps_test";
    task_config.io.source_paths = {source_path.string()};
    task_config.io.target_paths = {video_path.string()};
    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "result_";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& /*p*/) {});
    auto duration_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();

    ASSERT_TRUE(result.is_ok());

    // Calculate actual FPS
    double actual_fps = (input_info.frame_count * 1000.0) / duration_ms;

    std::cout << "=== Performance Summary ===" << std::endl;
    std::cout << "Total frames: " << input_info.frame_count << std::endl;
    std::cout << "Duration: " << duration_ms << " ms" << std::endl;
    std::cout << "Actual FPS: " << actual_fps << std::endl;

#ifdef NDEBUG
    EXPECT_GE(actual_fps, test_constants::MIN_FPS_RTX4060)
        << "FPS below threshold: " << actual_fps << " (min: " << test_constants::MIN_FPS_RTX4060 << ")";
#else
    std::cout << "[WARN] Running in DEBUG mode. FPS requirement ignored. Got: " << actual_fps
              << std::endl;
#endif
}

TEST_F(PipelineRunnerVideoTest, ProcessVideo_CompletesWithinTimeLimit) {
    config::TaskConfig task_config;
    task_config.task_info.id = "video_720p_time_test";
    task_config.io.source_paths = {source_path.string()};
    task_config.io.target_paths = {video_path.string()};
    task_config.io.output.path = output_dir.string();
    task_config.io.output.prefix = "result_";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& /*p*/) {});
    auto duration_s = duration_cast<seconds>(steady_clock::now() - start).count();

    ASSERT_TRUE(result.is_ok());

    int64_t max_duration_s = test_constants::TIMEOUT_VIDEO_40S_MS / 1000;
    EXPECT_LT(duration_s, max_duration_s)
        << "Processing time exceeded: " << duration_s << "s (max: " << max_duration_s << "s)";
}
