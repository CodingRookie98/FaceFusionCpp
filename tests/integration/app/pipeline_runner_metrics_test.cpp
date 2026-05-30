1 | #include < gtest / gtest.h > 2 | #include < nlohmann / json.hpp > 3 | #include < filesystem > 4
    | #include < fstream > 5 | #include < thread > 6 | #include < regex > 7 | 8
    | import services.pipeline.runner;
9 | import services.pipeline.metrics;
10 | import config.task;
11 | import config.app;
12 | 13 | using json = nlohmann::json;
14 | 15 | class PipelineRunnerMetricsTest : public ::testing::Test {
    16 | protected : 17 | void SetUp() override {
        18 | output_dir = std::filesystem::temp_directory_path() / "facefusion_test_metrics";
        19 | std::filesystem::create_directories(output_dir);
        20 | metrics_path = output_dir / "test_metrics.json";
        21 |
    }
    22 | 23 | void TearDown() override { std::filesystem::remove_all(output_dir); }
    24 | 25 | std::filesystem::path output_dir;
    26 | std::filesystem::path metrics_path;
    27 | 28 | json load_metrics_json() {
        29 | std::ifstream file(metrics_path);
        30 | EXPECT_TRUE(file.is_open()) << "Failed to open metrics file: " << metrics_path;
        31 | return json::parse(file);
        32 |
    }
    33 |
};
34 | 35 | TEST_F(PipelineRunnerMetricsTest, MetricsExportSchemaVersionMatches10) {
    36 | // Arrange
        37 | services::pipeline::MetricsCollector collector("test_task");
    38 | collector.set_total_frames(10);
    39 | for (int i = 0; i < 10; ++i) {
        collector.record_frame_completed();
    }
    40 | 41 | // Act
        42 | bool success = collector.export_json(metrics_path);
    43 | 44 | // Assert
        45 | ASSERT_TRUE(success);
    46 | ASSERT_TRUE(std::filesystem::exists(metrics_path));
    47 | 48 | auto metrics = load_metrics_json();
    49 | EXPECT_EQ(metrics["schema_version"], "1.0");
    50 |
}
51 | 52 | TEST_F(PipelineRunnerMetricsTest, MetricsExportSummaryFrameCountsCorrect) {
    53 | // Arrange
        54 | services::pipeline::MetricsCollector collector("test_task");
    55 | collector.set_total_frames(100);
    56 | 57 | for (int i = 0; i < 90; ++i) {
        collector.record_frame_completed();
    }
    58 | for (int i = 0; i < 5; ++i) {
        collector.record_frame_failed();
    }
    59 | for (int i = 0; i < 5; ++i) {
        collector.record_frame_skipped();
    }
    60 | 61 | // Act
        62 | collector.export_json(metrics_path);
    63 | auto metrics = load_metrics_json();
    64 | 65 | // Assert
        66 | EXPECT_EQ(metrics["summary"]["total_frames"], 100);
    67 | EXPECT_EQ(metrics["summary"]["processed_frames"], 90);
    68 | EXPECT_EQ(metrics["summary"]["failed_frames"], 5);
    69 |
}
70 | 71 | TEST_F(PipelineRunnerMetricsTest, MetricsExportStepLatencyRecordsMultipleSteps) {
    72 | // Arrange
        73 | services::pipeline::MetricsCollector collector("test_task");
    74 | 75 | // Simulate step timing
        76 | for (int i = 0; i < 10; ++i) {
        77 | {
            78 | services::pipeline::ScopedStepTimer timer(collector, "face_swap");
            79 | std::this_thread::sleep_for(std::chrono::milliseconds(5));
            80 |
        }
        81 | {
            82 | services::pipeline::ScopedStepTimer timer(collector, "face_enhance");
            83 | std::this_thread::sleep_for(std::chrono::milliseconds(3));
            84 |
        }
        85 |
    }
    86 | 87 | // Act
        88 | collector.export_json(metrics_path);
    89 | auto metrics = load_metrics_json();
    90 | 91 | // Assert
        92 | auto& steps = metrics["step_latency"];
    93 | EXPECT_EQ(steps.size(), 2);
    94 | 95 | bool has_swap = false, has_enhance = false;
    96 | for (const auto& step : steps) {
        97 | if (step["step_name"] == "face_swap") {
            98 | has_swap = true;
            99 | EXPECT_GT(step["avg_ms"].get<double>(), 0);
            100 |
        }
        101 | if (step["step_name"] == "face_enhance") {
            has_enhance = true;
        }
        102 |
    }
    103 | EXPECT_TRUE(has_swap);
    104 | EXPECT_TRUE(has_enhance);
    105 |
}
106 | 107 | TEST_F(PipelineRunnerMetricsTest, MetricsExportTimestampIsISO8601) {
    108 | // Arrange
        109 | services::pipeline::MetricsCollector collector("test_task");
    110 | collector.export_json(metrics_path);
    111 | 112 | // Act
        113 | auto metrics = load_metrics_json();
    114 | std::string timestamp = metrics["timestamp"];
    115 | 116 | // Assert - ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
        117 | std::regex iso8601_pattern(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)");
    118 | EXPECT_TRUE(std::regex_match(timestamp, iso8601_pattern)) 119 |
        << "Timestamp not in ISO 8601 format: " << timestamp;
    120 |
}
121 |