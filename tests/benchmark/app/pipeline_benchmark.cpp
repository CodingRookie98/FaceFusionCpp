/**
 * @file pipeline_benchmark_test.cpp
 * @brief Benchmark tests for PipelineRunner to measure performance
 * @author Sisyphus
 * @date 2026-01-29
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>
#include <opencv2/opencv.hpp>

import services.pipeline.runner;
import config.task;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;
import foundation.media.ffmpeg;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;

class PipelineBenchmarkTest : public ::testing::Test {
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

        // Ensure output directory exists
        std::filesystem::create_directories("tests_output/benchmark");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path video_path;
};

TEST_F(PipelineBenchmarkTest, BenchmarkVideoProcessing) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found.";
    }

    config::AppConfig app_config;
    // Optional: Configure app_config for performance (e.g., enable TensorRT if available)

    auto runner = CreatePipelineRunner(app_config);

    config::TaskConfig task_config;
    task_config.config_version = "1.0";
    task_config.task_info.id = "benchmark_video";
    task_config.io.source_paths.push_back(source_path.string());
    task_config.io.target_paths.push_back(video_path.string());

    task_config.io.output.path = "tests_output/benchmark";
    task_config.io.output.prefix = "bench_";
    task_config.io.output.suffix = "";

    // Sequential execution for now (Batch mode is not fully parallelized yet in runner)
    task_config.resource.execution_order = config::ExecutionOrder::Sequential;
    task_config.resource.max_frames = 20; // Limit to 20 frames for benchmark baseline

    // Full Pipeline: Swapper + Enhancer
    // This simulates a heavy workload
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

    // Cleanup previous run
    std::string output_file = "tests_output/benchmark/bench_slideshow_scaled.mp4";
    if (std::filesystem::exists(output_file)) { std::filesystem::remove(output_file); }

    std::cout << "[BENCHMARK] Starting Pipeline Benchmark..." << std::endl;
    std::cout << "[BENCHMARK] Source: " << source_path << std::endl;
    std::cout << "[BENCHMARK] Target: " << video_path << std::endl;
    std::cout << "[BENCHMARK] Pipeline: Swapper -> Enhancer" << std::endl;

    // --- TIMING START ---
    auto start_time = std::chrono::high_resolution_clock::now();

    auto result = runner->Run(task_config, [](const services::pipeline::TaskProgress& p) {
        // Optional: Print progress sparingly
        // if (static_cast<int>(p.progress * 100) % 20 == 0) {
        //     std::cout << "." << std::flush;
        // }
    });

    auto end_time = std::chrono::high_resolution_clock::now();
    // --- TIMING END ---

    ASSERT_TRUE(result.is_ok()) << "Benchmark run failed: " << result.error().message;

    auto duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Get video info to calculate FPS
    foundation::media::ffmpeg::VideoReader reader(video_path.string());
    int total_frames = 0;
    if (reader.open()) { total_frames = reader.get_frame_count(); }

    double fps = 0.0;
    if (total_frames > 0 && duration_ms > 0) {
        fps = (static_cast<double>(total_frames) / duration_ms) * 1000.0;
    }

    std::cout << "\n=======================================================" << std::endl;
    std::cout << "[BENCHMARK RESULT]" << std::endl;
    std::cout << "Total Frames: " << total_frames << std::endl;
    std::cout << "Total Time  : " << duration_ms << " ms" << std::endl;
    std::cout << "Average FPS : " << fps << std::endl;
    std::cout << "=======================================================\n" << std::endl;
}
