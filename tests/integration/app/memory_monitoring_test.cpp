1 | #include < gtest / gtest.h > 2 | #include < filesystem > 3 | #include < thread > 4
    | #include < chrono > 5 | #include < iostream > 6 | 7
    | import tests.helpers.foundation.nvml_monitor;
8 | import tests.helpers.foundation.memory_monitor;
9 | 10 | import services.pipeline.runner;
11 | import config.app;
12 | import config.task;
13 | import config.merger;
14 | import config.types;
15 | import tests.helpers.foundation.test_utilities;
16 | import domain.ai.model_repository;
17 | 18 | using namespace tests::helpers::foundation;
19 | 20 | extern void LinkGlobalTestEnvironment();
21 | 22 | class MemoryMonitoringTest : public ::testing::Test {
    23 | protected : 24 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    25 | 26 | void SetUp() override {
        27 | auto repo = domain::ai::model_repository::ModelRepository::get_instance();
        28 | auto assets_path = get_assets_path();
        29 | repo->set_model_info_file_path((assets_path / "models_info.json").string());
        30 | 31 | // Ensure assets exist
            32 | video_path_ = assets_path / "standard_face_test_videos" / "slideshow_scaled.mp4";
        33 | image_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        34 | output_dir_ = "test_output/";
        35 | 36 | if (!std::filesystem::exists(video_path_)) {
            37 | std::cerr << "Warning: Video path does not exist: " << video_path_ << std::endl;
            38 |
        }
        39 |
    }
    40 | 41 | void TearDown() override {
        42 | // Cleanup outputs
            43 |
    }
    44 | 45 | std::filesystem::path video_path_;
    46 | std::filesystem::path image_path_;
    47 | std::filesystem::path output_dir_;
    48 | 49 | // Helper to run a pipeline task
        50
        | void RunTask(const std::string& task_id, const std::string& input_file,
                       51 | const std::string& output_prefix) {
        52 | config::AppConfig app_config;
        53 | config::TaskConfig task_config;
        54 | 55 | task_config.task_info.id = task_id;
        56 | task_config.task_info.enable_logging = true;
        57 | 58 | task_config.io.source_paths = {image_path_.string()};
        59 | task_config.io.target_paths = {input_file};
        60 | task_config.io.output.path = output_dir_.string();
        61 | task_config.io.output.prefix = output_prefix;
        62 | task_config.io.output.conflict_policy = config::ConflictPolicy::Overwrite;
        63 | task_config.io.output.image_format = "jpg"; // Default
        64 | 65 |                                        // Simple pipeline: Swapper
            66 | config::PipelineStep step;
        67 | step.step = "face_swapper";
        68 | step.enabled = true;
        69 | config::FaceSwapperParams params;
        70 | params.face_selector_mode = config::FaceSelectorMode::Many;
        71 | params.model = "inswapper_128_fp16";
        72 | step.params = params;
        73 | task_config.pipeline.push_back(step);
        74 | 75 | auto merged_config = config::MergeConfigs(task_config, app_config);
        76 | 77 | services::pipeline::PipelineRunner runner(app_config);
        78 | auto result = runner.run(merged_config);
        79 | if (!result.is_ok()) {
            FAIL() << "Pipeline failed: " << result.error().message;
        }
        80 |
    }
    81 |
};
82 | 83 | TEST_F(MemoryMonitoringTest, VRAMPeakBelowThresholdDuringVideoProcessing) {
    84 | #ifndef HAVE_NVML 85 | GTEST_SKIP() << "NVML not available, skipping VRAM test";
    86 | #endif 87 | 88 | tests::helpers::foundation::NvmlMonitor nvml_monitor;
    89 | nvml_monitor.start();
    90 | 91 | if (!std::filesystem::exists(video_path_)) {
        92 | GTEST_SKIP() << "Test video not found at " << video_path_;
        93 |
    }
    94 | 95 | RunTask("vram_test_video", video_path_.string(), "vram_test_");
    96 | 97 | nvml_monitor.stop();
    98 | 99 | double peak_gb = nvml_monitor.get_peak_used_gb();
    100 | std::cout << "Peak VRAM Usage: " << peak_gb << " GB" << std::endl;
    101 | 102 | // Threshold from acceptance criteria: < 6.5 GB for RTX 4060
        103 | EXPECT_LT(peak_gb, 6.5);
    104 |
}
105 | 106 | TEST_F(MemoryMonitoringTest, MemoryLeakDeltaBelowThresholdAfterProcessing) {
    107 | RunTask("warmup", image_path_.string(), "warmup_");
    108 | 109 | tests::helpers::foundation::MemoryDeltaChecker ram_checker;
    110 | 111 | // Run multiple times to amplify leak if any
        112 | for (int i = 0; i < 5; ++i) {
        113
            | RunTask("mem_leak_test_" + std::to_string(i), image_path_.string(),
                      114 | "leak_test_" + std::to_string(i) + "_");
        115 |
    }
    116 | 117 | double delta_mb = ram_checker.get_rss_delta_mb();
    118 | std::cout << "Memory Delta (after warmup): " << delta_mb << " MB" << std::endl;
    119 | 120 | // Threshold: < 50MB
        121 | EXPECT_LT(delta_mb, 50.0);
    122 |
}
123 |