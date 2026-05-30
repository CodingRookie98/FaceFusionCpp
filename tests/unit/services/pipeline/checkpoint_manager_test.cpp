#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

import services.pipeline.checkpoint;

namespace fs = std::filesystem;
using namespace services::pipeline;

class CheckpointManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "./test_checkpoints";
        if (fs::exists(test_dir)) fs::remove_all(test_dir);
        mgr = std::make_unique<CheckpointManager>(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) fs::remove_all(test_dir);
    }

    fs::path test_dir;
    std::unique_ptr<CheckpointManager> mgr;
};

TEST_F(CheckpointManagerTest, SaveAndLoad) {
    CheckpointData data;
    data.task_id = "test_task";
    data.config_hash = "hash123";
    data.last_completed_frame = 100;
    data.total_frames = 1000;
    data.output_path = "output.mp4";

    mgr->force_save(data);

    auto loaded = mgr->load("test_task", "hash123");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->task_id, "test_task");
    EXPECT_EQ(loaded->last_completed_frame, 100);
    EXPECT_EQ(loaded->total_frames, 1000);
    EXPECT_EQ(loaded->output_path, "output.mp4");
}

TEST_F(CheckpointManagerTest, ConfigMismatch) {
    CheckpointData data;
    data.task_id = "test_task";
    data.config_hash = "hash123";
    mgr->force_save(data);

    // Load with different hash
    auto loaded = mgr->load("test_task", "different_hash");
    EXPECT_FALSE(loaded.has_value());
}

TEST_F(CheckpointManagerTest, Cleanup) {
    CheckpointData data;
    data.task_id = "test_task";
    mgr->force_save(data);
    ASSERT_TRUE(mgr->exists("test_task"));

    mgr->cleanup("test_task");
    EXPECT_FALSE(mgr->exists("test_task"));
}

TEST_F(CheckpointManagerTest, IntegrityCheck) {
    CheckpointData data;
    data.task_id = "test_task";
    mgr->force_save(data);

    // Corrupt the file manually
    auto path = mgr->get_checkpoint_path("test_task");
    std::ofstream ofs(path, std::ios::app);
    ofs << "corrupted";
    ofs.close();

    auto loaded = mgr->load("test_task");
    EXPECT_FALSE(loaded.has_value());
}
