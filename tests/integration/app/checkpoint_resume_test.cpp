1 | #include < gtest / gtest.h > 2 | #include < filesystem > 3 | #include < thread > 4
    | #include < atomic > 5 | #include < chrono > 6 | #include < opencv2 / opencv.hpp > 7
    | #include < fstream > 8 | 9 | import services.pipeline.runner;
10 | import services.pipeline.checkpoint;
11 | import config.task;
12 | import config.app;
13 | import tests.helpers.foundation.test_utilities;
14 | import foundation.infrastructure.crypto;
15 | 16 | using namespace services::pipeline;
17 | using namespace tests::helpers::foundation;
18 | using namespace std::chrono;
19 | 20 | extern void LinkGlobalTestEnvironment();
21 | 22 | class CheckpointResumeTest : public ::testing::Test {
    23 | protected : 24 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    25 | 26 | void SetUp() override {
        27 | checkpoint_dir_ = std::filesystem::temp_directory_path() / "checkpoint_test";
        28 | output_dir_ = std::filesystem::temp_directory_path() / "checkpoint_output";
        29 | 30 | std::filesystem::create_directories(checkpoint_dir_);
        31 | std::filesystem::create_directories(output_dir_);
        32 | 33 | auto assets_path = get_assets_path();
        34 | source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        35 | video_path_ = assets_path / "standard_face_test_videos" / "slideshow_scaled.mp4";
        36 |
    }
    37 | 38 | void TearDown() override {
        39 | std::filesystem::remove_all(checkpoint_dir_);
        40 | std::filesystem::remove_all(output_dir_);
        41 |
    }
    42 | 43 | std::filesystem::path checkpoint_dir_;
    44 | std::filesystem::path output_dir_;
    45 | std::filesystem::path source_path_;
    46 | std::filesystem::path video_path_;
    47 | 48 | config::TaskConfig create_test_config(const std::string& task_id) {
        49 | config::TaskConfig config;
        50 | config.task_info.id = task_id;
        51 | config.task_info.enable_resume = true;
        52 | config.io.source_paths = {source_path_.string()};
        53 | config.io.target_paths = {video_path_.string()};
        54 | config.io.output.path = output_dir_.string();
        55 | 56 | config::PipelineStep swap_step;
        57 | swap_step.step = "face_swapper";
        58 | swap_step.enabled = true;
        59 | config.pipeline.push_back(swap_step);
        60 | 61 | return config;
        62 |
    }
    63 |
};
64 | 65 | // ============================================================================
    66 |  // 场景 1: 正常中断恢复
    67 |  // ============================================================================
    68 | 69 | TEST_F(CheckpointResumeTest, ResumeAfterInterruptionContinuesFromLastFrame) {
    70 | // Arrange
        71 | const std::string task_id = "resume_test_001";
    72 | CheckpointManager ckpt_mgr(checkpoint_dir_);
    73 | 74 | // 模拟部分处理完成后的 Checkpoint
        75 | CheckpointData initial_ckpt;
    76 | initial_ckpt.task_id = task_id;
    77 | initial_ckpt.last_completed_frame = 100; // 假设已处理 100 帧
    78 | initial_ckpt.total_frames = 491;
    79 | initial_ckpt.output_path = (output_dir_ / "result.mp4").string();
    80 | initial_ckpt.config_hash = "test_config_hash";
    81 | ckpt_mgr.force_save(initial_ckpt);
    82 | 83 | // 验证 Checkpoint 已保存
        84 | ASSERT_TRUE(ckpt_mgr.exists(task_id));
    85 | 86 | // Act - 加载 Checkpoint
        87 | auto loaded_ckpt = ckpt_mgr.load(task_id, "test_config_hash");
    88 | 89 | // Assert
        90 | ASSERT_TRUE(loaded_ckpt.has_value());
    91 | EXPECT_EQ(loaded_ckpt->last_completed_frame, 100);
    92 | EXPECT_EQ(loaded_ckpt->total_frames, 491);
    93 | 94 | // 验证恢复起始帧应为 101
        95 | int64_t resume_from = loaded_ckpt->last_completed_frame + 1;
    96 | EXPECT_EQ(resume_from, 101);
    97 |
}
98 | 99 | // ============================================================================
    100 | // 场景 2: Checkpoint 完整性验证
    101 | // ============================================================================
    102 | 103 | TEST_F(CheckpointResumeTest, LoadCorruptedCheckpointReturnsNullopt) {
    104 | // Arrange
        105 | const std::string task_id = "corrupt_test";
    106 | CheckpointManager ckpt_mgr(checkpoint_dir_);
    107 | 108 | // 手动写入损坏的 Checkpoint 文件
        109 | auto ckpt_path = ckpt_mgr.get_checkpoint_path(task_id);
    110 | // Ensure parent dir exists
        111 | std::filesystem::create_directories(ckpt_path.parent_path());
    112 | 113 | {
        114 | std::ofstream file(ckpt_path);
        115 | file << "{\"task_id\":\"corrupt_test\",\"checksum\":\"invalid_checksum\"}";
        116 |
    }
    117 | 118 | // Act
        119 | auto loaded = ckpt_mgr.load(task_id);
    120 | 121 | // Assert - 校验和不匹配应返回 nullopt
        122 | EXPECT_FALSE(loaded.has_value());
    123 |
}
124 | 125 | TEST_F(CheckpointResumeTest, LoadConfigHashMismatchReturnsNullopt) {
    126 | // Arrange
        127 | const std::string task_id = "config_mismatch_test";
    128 | CheckpointManager ckpt_mgr(checkpoint_dir_);
    129 | 130 | CheckpointData ckpt;
    131 | ckpt.task_id = task_id;
    132 | ckpt.config_hash = "original_hash";
    133 | ckpt.last_completed_frame = 50;
    134 | ckpt_mgr.force_save(ckpt);
    135 | 136 | // Act - 使用不同的 config_hash 加载
        137 | auto loaded = ckpt_mgr.load(task_id, "different_hash");
    138 | 139 | // Assert - 配置哈希不匹配应返回 nullopt
        140 | EXPECT_FALSE(loaded.has_value());
    141 |
}
142 | 143 | // ============================================================================
    144 |   // 场景 3: 任务完成后自动清理
    145 |   // ============================================================================
    146 | 147 | TEST_F(CheckpointResumeTest, CleanupAfterCompletionRemovesCheckpointFile) {
    148 | // Arrange
        149 | const std::string task_id = "cleanup_test";
    150 | CheckpointManager ckpt_mgr(checkpoint_dir_);
    151 | 152 | CheckpointData ckpt;
    153 | ckpt.task_id = task_id;
    154 | ckpt.last_completed_frame = 490;
    155 | ckpt.total_frames = 491;
    156 | ckpt_mgr.force_save(ckpt);
    157 | 158 | ASSERT_TRUE(ckpt_mgr.exists(task_id));
    159 | 160 | // Act
        161 | ckpt_mgr.cleanup(task_id);
    162 | 163 | // Assert
        164 | EXPECT_FALSE(ckpt_mgr.exists(task_id));
    165 | EXPECT_FALSE(std::filesystem::exists(ckpt_mgr.get_checkpoint_path(task_id)));
    166 |
}
167 | 168 | // ============================================================================
    169 |   // 场景 4: 周期性保存
    170 |   // ============================================================================
    171 | 172 | TEST_F(CheckpointResumeTest, SaveRespectsMinInterval) {
    173 | // Arrange
        174 | const std::string task_id = "interval_test";
    175 | CheckpointManager ckpt_mgr(checkpoint_dir_);
    176 | 177 | CheckpointData ckpt;
    178 | ckpt.task_id = task_id;
    179 | ckpt.last_completed_frame = 10;
    180 | 181 | // Act - 快速连续保存
        182 |   // First save: should succeed
        183 | bool first_save = ckpt_mgr.save(ckpt, std::chrono::seconds{5});
    184 | 185 | ckpt.last_completed_frame = 20;
    186 | // Second save immediately after: should be skipped
        187 | bool second_save = ckpt_mgr.save(ckpt, std::chrono::seconds{5});
    188 | 189 | // Assert - 第一次保存成功，第二次因间隔太短被跳过
        190 | EXPECT_TRUE(first_save);
    191 | EXPECT_FALSE(second_save);
    192 | 193 | // 验证保存的是第一次的数据
        194 | auto loaded = ckpt_mgr.load(task_id);
    195 | ASSERT_TRUE(loaded.has_value());
    196 | EXPECT_EQ(loaded->last_completed_frame, 10);
    197 |
}
198 |