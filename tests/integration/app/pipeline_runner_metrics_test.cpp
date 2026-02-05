#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <regex>

import services.pipeline.runner;
import services.pipeline.metrics;
import config.task;
import config.app;

using json = nlohmann::json;

class PipelineRunnerMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        output_dir = std::filesystem::temp_directory_path() / "facefusion_test_metrics";
        std::filesystem::create_directories(output_dir);
        metrics_path = output_dir / "test_metrics.json";
    }
    
    void TearDown() override {
        std::filesystem::remove_all(output_dir);
    }
    
    std::filesystem::path output_dir;
    std::filesystem::path metrics_path;
    
    json load_metrics_json() {
        std::ifstream file(metrics_path);
        EXPECT_TRUE(file.is_open()) << "Failed to open metrics file: " << metrics_path;
        return json::parse(file);
    }
};

TEST_F(PipelineRunnerMetricsTest, MetricsExport_SchemaVersion_Matches1_0) {
    // Arrange
    services::pipeline::MetricsCollector collector("test_task");
    collector.set_total_frames(10);
    for (int i = 0; i < 10; ++i) {
        collector.record_frame_completed();
    }
    
    // Act
    bool success = collector.export_json(metrics_path);
    
    // Assert
    ASSERT_TRUE(success);
    ASSERT_TRUE(std::filesystem::exists(metrics_path));
    
    auto metrics = load_metrics_json();
    EXPECT_EQ(metrics["schema_version"], "1.0");
}

TEST_F(PipelineRunnerMetricsTest, MetricsExport_Summary_FrameCountsCorrect) {
    // Arrange
    services::pipeline::MetricsCollector collector("test_task");
    collector.set_total_frames(100);
    
    for (int i = 0; i < 90; ++i) { collector.record_frame_completed(); }
    for (int i = 0; i < 5; ++i) { collector.record_frame_failed(); }
    for (int i = 0; i < 5; ++i) { collector.record_frame_skipped(); }
    
    // Act
    collector.export_json(metrics_path);
    auto metrics = load_metrics_json();
    
    // Assert
    EXPECT_EQ(metrics["summary"]["total_frames"], 100);
    EXPECT_EQ(metrics["summary"]["processed_frames"], 90);
    EXPECT_EQ(metrics["summary"]["failed_frames"], 5);
}

TEST_F(PipelineRunnerMetricsTest, MetricsExport_StepLatency_RecordsMultipleSteps) {
    // Arrange
    services::pipeline::MetricsCollector collector("test_task");
    
    // Simulate step timing
    for (int i = 0; i < 10; ++i) {
        {
            services::pipeline::ScopedStepTimer timer(collector, "face_swap");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        {
            services::pipeline::ScopedStepTimer timer(collector, "face_enhance");
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
    
    // Act
    collector.export_json(metrics_path);
    auto metrics = load_metrics_json();
    
    // Assert
    auto& steps = metrics["step_latency"];
    EXPECT_EQ(steps.size(), 2);
    
    bool has_swap = false, has_enhance = false;
    for (const auto& step : steps) {
        if (step["step_name"] == "face_swap") {
            has_swap = true;
            EXPECT_GT(step["avg_ms"].get<double>(), 0);
        }
        if (step["step_name"] == "face_enhance") {
            has_enhance = true;
        }
    }
    EXPECT_TRUE(has_swap);
    EXPECT_TRUE(has_enhance);
}

TEST_F(PipelineRunnerMetricsTest, MetricsExport_Timestamp_IsISO8601) {
    // Arrange
    services::pipeline::MetricsCollector collector("test_task");
    collector.export_json(metrics_path);
    
    // Act
    auto metrics = load_metrics_json();
    std::string timestamp = metrics["timestamp"];
    
    // Assert - ISO 8601 format: YYYY-MM-DDTHH:MM:SSZ
    std::regex iso8601_pattern(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)");
    EXPECT_TRUE(std::regex_match(timestamp, iso8601_pattern))
        << "Timestamp not in ISO 8601 format: " << timestamp;
}
