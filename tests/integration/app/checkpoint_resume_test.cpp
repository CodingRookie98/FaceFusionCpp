#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <fstream>

import services.pipeline.runner;
import services.pipeline.checkpoint;
import config.task;
import config.app;
import foundation.infrastructure.test_support;
import foundation.infrastructure.crypto;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;
using namespace std::chrono;

extern void LinkGlobalTestEnvironment();

class CheckpointResumeTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        checkpoint_dir_ = std::filesystem::temp_directory_path() / "checkpoint_test";
        output_dir_ = std::filesystem::temp_directory_path() / "checkpoint_output";

        std::filesystem::create_directories(checkpoint_dir_);
        std::filesystem::create_directories(output_dir_);

        auto assets_path = get_assets_path();
        source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        video_path_ = assets_path / "standard_face_test_videos" / "slideshow_scaled.mp4";
    }

    void TearDown() override {
        std::filesystem::remove_all(checkpoint_dir_);
        std::filesystem::remove_all(output_dir_);
    }

    std::filesystem::path checkpoint_dir_;
    std::filesystem::path output_dir_;
    std::filesystem::path source_path_;
    std::filesystem::path video_path_;

    config::TaskConfig create_test_config(const std::string& task_id) {
        config::TaskConfig config;
        config.task_info.id = task_id;
        config.task_info.enable_resume = true;
        config.io.source_paths = {source_path_.string()};
        config.io.target_paths = {video_path_.string()};
        config.io.output.path = output_dir_.string();

        config::PipelineStep swap_step;
        swap_step.step = "face_swapper";
        swap_step.enabled = true;
        config.pipeline.push_back(swap_step);

        return config;
    }
};

// ============================================================================
// 场景 1: 正常中断恢复
// ============================================================================

TEST_F(CheckpointResumeTest, Resume_AfterInterruption_ContinuesFromLastFrame) {
    // Arrange
    const std::string task_id = "resume_test_001";
    CheckpointManager ckpt_mgr(checkpoint_dir_);

    // 模拟部分处理完成后的 Checkpoint
    CheckpointData initial_ckpt;
    initial_ckpt.task_id = task_id;
    initial_ckpt.last_completed_frame = 100; // 假设已处理 100 帧
    initial_ckpt.total_frames = 491;
    initial_ckpt.output_path = (output_dir_ / "result.mp4").string();
    initial_ckpt.config_hash = "test_config_hash";
    ckpt_mgr.force_save(initial_ckpt);

    // 验证 Checkpoint 已保存
    ASSERT_TRUE(ckpt_mgr.exists(task_id));

    // Act - 加载 Checkpoint
    auto loaded_ckpt = ckpt_mgr.load(task_id, "test_config_hash");

    // Assert
    ASSERT_TRUE(loaded_ckpt.has_value());
    EXPECT_EQ(loaded_ckpt->last_completed_frame, 100);
    EXPECT_EQ(loaded_ckpt->total_frames, 491);

    // 验证恢复起始帧应为 101
    int64_t resume_from = loaded_ckpt->last_completed_frame + 1;
    EXPECT_EQ(resume_from, 101);
}

// ============================================================================
// 场景 2: Checkpoint 完整性验证
// ============================================================================

TEST_F(CheckpointResumeTest, Load_CorruptedCheckpoint_ReturnsNullopt) {
    // Arrange
    const std::string task_id = "corrupt_test";
    CheckpointManager ckpt_mgr(checkpoint_dir_);

    // 手动写入损坏的 Checkpoint 文件
    auto ckpt_path = ckpt_mgr.get_checkpoint_path(task_id);
    // Ensure parent dir exists
    std::filesystem::create_directories(ckpt_path.parent_path());

    {
        std::ofstream file(ckpt_path);
        file << "{\"task_id\":\"corrupt_test\",\"checksum\":\"invalid_checksum\"}";
    }

    // Act
    auto loaded = ckpt_mgr.load(task_id);

    // Assert - 校验和不匹配应返回 nullopt
    EXPECT_FALSE(loaded.has_value());
}

TEST_F(CheckpointResumeTest, Load_ConfigHashMismatch_ReturnsNullopt) {
    // Arrange
    const std::string task_id = "config_mismatch_test";
    CheckpointManager ckpt_mgr(checkpoint_dir_);

    CheckpointData ckpt;
    ckpt.task_id = task_id;
    ckpt.config_hash = "original_hash";
    ckpt.last_completed_frame = 50;
    ckpt_mgr.force_save(ckpt);

    // Act - 使用不同的 config_hash 加载
    auto loaded = ckpt_mgr.load(task_id, "different_hash");

    // Assert - 配置哈希不匹配应返回 nullopt
    EXPECT_FALSE(loaded.has_value());
}

// ============================================================================
// 场景 3: 任务完成后自动清理
// ============================================================================

TEST_F(CheckpointResumeTest, Cleanup_AfterCompletion_RemovesCheckpointFile) {
    // Arrange
    const std::string task_id = "cleanup_test";
    CheckpointManager ckpt_mgr(checkpoint_dir_);

    CheckpointData ckpt;
    ckpt.task_id = task_id;
    ckpt.last_completed_frame = 490;
    ckpt.total_frames = 491;
    ckpt_mgr.force_save(ckpt);

    ASSERT_TRUE(ckpt_mgr.exists(task_id));

    // Act
    ckpt_mgr.cleanup(task_id);

    // Assert
    EXPECT_FALSE(ckpt_mgr.exists(task_id));
    EXPECT_FALSE(std::filesystem::exists(ckpt_mgr.get_checkpoint_path(task_id)));
}

// ============================================================================
// 场景 4: 周期性保存
// ============================================================================

TEST_F(CheckpointResumeTest, Save_RespectsMinInterval) {
    // Arrange
    const std::string task_id = "interval_test";
    CheckpointManager ckpt_mgr(checkpoint_dir_);

    CheckpointData ckpt;
    ckpt.task_id = task_id;
    ckpt.last_completed_frame = 10;

    // Act - 快速连续保存
    // First save: should succeed
    bool first_save = ckpt_mgr.save(ckpt, std::chrono::seconds{5});

    ckpt.last_completed_frame = 20;
    // Second save immediately after: should be skipped
    bool second_save = ckpt_mgr.save(ckpt, std::chrono::seconds{5});

    // Assert - 第一次保存成功，第二次因间隔太短被跳过
    EXPECT_TRUE(first_save);
    EXPECT_FALSE(second_save);

    // 验证保存的是第一次的数据
    auto loaded = ckpt_mgr.load(task_id);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->last_completed_frame, 10);
}
