#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <iostream>

#include "test_support/nvml_monitor.hpp"
#include "test_support/memory_monitor.hpp"

import services.pipeline.runner;
import config.app;
import config.task;
import config.merger;
import config.types;
import foundation.infrastructure.test_support;
import domain.ai.model_repository;

using namespace foundation::infrastructure::test;

class MemoryMonitoringTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto repo = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        repo->set_model_info_file_path((assets_path / "models_info.json").string());

        // Ensure assets exist
        video_path_ = assets_path / "standard_face_test_videos" / "slideshow_scaled.mp4";
        image_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        output_dir_ = "test_output/";
        
        if (!std::filesystem::exists(video_path_)) {
             std::cerr << "Warning: Video path does not exist: " << video_path_ << std::endl;
        }
    }
    
    void TearDown() override {
        // Cleanup outputs
    }
    
    std::filesystem::path video_path_;
    std::filesystem::path image_path_;
    std::filesystem::path output_dir_;
    
    // Helper to run a pipeline task
    void RunTask(const std::string& task_id, const std::string& input_file, const std::string& output_prefix) {
        config::AppConfig app_config;
        config::TaskConfig task_config;
        
        task_config.task_info.id = task_id;
        task_config.task_info.enable_logging = true;
        
        task_config.io.source_paths = {image_path_.string()};
        task_config.io.target_paths = {input_file};
        task_config.io.output.path = output_dir_.string();
        task_config.io.output.prefix = output_prefix;
        task_config.io.output.conflict_policy = config::ConflictPolicy::Overwrite;
        task_config.io.output.image_format = "jpg"; // Default
        
        // Simple pipeline: Swapper
        config::PipelineStep step;
        step.step = "face_swapper";
        step.enabled = true;
        config::FaceSwapperParams params;
        params.face_selector_mode = config::FaceSelectorMode::Many;
        params.model = "inswapper_128_fp16";
        step.params = params;
        task_config.pipeline.push_back(step);
        
        auto merged_config = config::MergeConfigs(task_config, app_config);
        
        services::pipeline::PipelineRunner runner(app_config);
        auto result = runner.Run(merged_config);
        if (!result.is_ok()) {
            FAIL() << "Pipeline failed: " << result.error().message;
        }
    }
};

TEST_F(MemoryMonitoringTest, VRAMPeak_BelowThreshold_DuringVideoProcessing) {
    #ifndef HAVE_NVML
        GTEST_SKIP() << "NVML not available, skipping VRAM test";
    #endif
    
    test_support::NvmlMonitor nvml_monitor;
    nvml_monitor.start();
    
    if (!std::filesystem::exists(video_path_)) {
        GTEST_SKIP() << "Test video not found at " << video_path_;
    }
    
    RunTask("vram_test_video", video_path_.string(), "vram_test_");
    
    nvml_monitor.stop();
    
    double peak_gb = nvml_monitor.get_peak_used_gb();
    std::cout << "Peak VRAM Usage: " << peak_gb << " GB" << std::endl;
    
    // Threshold from acceptance criteria: < 6.5 GB for RTX 4060
    EXPECT_LT(peak_gb, 6.5); 
}

TEST_F(MemoryMonitoringTest, MemoryLeak_DeltaBelowThreshold_AfterProcessing) {
    RunTask("warmup", image_path_.string(), "warmup_");
    
    test_support::MemoryDeltaChecker ram_checker;
    
    // Run multiple times to amplify leak if any
    for (int i = 0; i < 5; ++i) {
        RunTask("mem_leak_test_" + std::to_string(i), image_path_.string(), "leak_test_" + std::to_string(i) + "_");
    }
    
    double delta_mb = ram_checker.get_rss_delta_mb();
    std::cout << "Memory Delta (after warmup): " << delta_mb << " MB" << std::endl;
    
    // Threshold: < 50MB
    EXPECT_LT(delta_mb, 50.0);
}
