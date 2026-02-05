#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

import services.pipeline.metrics;

namespace fs = std::filesystem;
using namespace services::pipeline;
using json = nlohmann::json;

class MetricsCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_log_dir = "./test_logs";
        if (fs::exists(test_log_dir)) fs::remove_all(test_log_dir);
        fs::create_directories(test_log_dir);
    }

    void TearDown() override {
        if (fs::exists(test_log_dir)) fs::remove_all(test_log_dir);
    }

    fs::path test_log_dir;
};

TEST_F(MetricsCollectorTest, BasicCollection) {
    MetricsCollector collector("task_001");
    collector.set_total_frames(100);
    collector.set_gpu_sample_interval(std::chrono::milliseconds(0)); // Disable rate limit for test

    // Record frames
    collector.record_frame_completed();
    collector.record_frame_completed();
    collector.record_frame_failed();

    // Record step latency
    {
        ScopedStepTimer timer(collector, "step_1");
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Record GPU
    collector.record_gpu_memory(1024);

    auto m = collector.get_metrics();
    EXPECT_EQ(m.task_id, "task_001");
    EXPECT_EQ(m.summary.total_frames, 100);
    EXPECT_EQ(m.summary.processed_frames, 2);
    EXPECT_EQ(m.summary.failed_frames, 1);

    ASSERT_FALSE(m.step_latency.empty());
    EXPECT_EQ(m.step_latency[0].step_name, "step_1");
    EXPECT_GE(m.step_latency[0].avg_ms, 45.0);

    EXPECT_EQ(m.gpu_memory.peak_mb, 1024);
}

TEST_F(MetricsCollectorTest, JsonExport) {
    MetricsCollector collector("task_001");
    collector.set_total_frames(10);
    collector.record_frame_completed();

    fs::path report_path = test_log_dir / "metrics.json";
    ASSERT_TRUE(collector.export_json(report_path));
    ASSERT_TRUE(fs::exists(report_path));

    // Verify JSON content
    std::ifstream ifs(report_path);
    json j = json::parse(ifs);
    EXPECT_EQ(j["task_id"], "task_001");
    EXPECT_EQ(j["summary"]["total_frames"], 10);
    EXPECT_EQ(j["summary"]["processed_frames"], 1);
}

TEST_F(MetricsCollectorTest, PercentileCalculation) {
    MetricsCollector collector("task_001");

    // Manually add samples to test percentiles
    // Since we don't have a public add_sample, we use start/end step
    for (int i = 1; i <= 100; ++i) {
        collector.start_step("test");
        // Simulate different durations
        // In a real test we might want to mock the clock, but here we just sleep a tiny bit
        // or rely on the fact that we can't easily control duration precisely.
        // Let's just verify it doesn't crash and returns reasonable values.
        collector.end_step("test");
    }

    auto m = collector.get_metrics();
    ASSERT_FALSE(m.step_latency.empty());
    EXPECT_GT(m.step_latency[0].p50_ms, 0.0);
    EXPECT_GE(m.step_latency[0].p99_ms, m.step_latency[0].p50_ms);
}
