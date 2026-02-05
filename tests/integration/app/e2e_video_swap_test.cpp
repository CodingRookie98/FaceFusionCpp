// tests/integration/app/e2e_video_swap_test.cpp

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <iostream>

import services.pipeline.runner;
import config.task;
import config.app;
import config.merger;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;
import domain.face.analyser;
import domain.face.test_support;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;
using namespace std::chrono;

class E2EVideoSwapTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo_ = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        repo_->set_model_info_file_path((assets_path / "models_info.json").string());

        source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        video_path_ = assets_path / "standard_face_test_videos" / "slideshow_scaled.mp4";

        output_dir_ = std::filesystem::current_path() / "tests_output" / "e2e_video_swap_test";
        std::filesystem::create_directories(output_dir_);
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo_;
    std::filesystem::path source_path_;
    std::filesystem::path video_path_;
    std::filesystem::path output_dir_;

    // 获取视频信息
    struct VideoInfo {
        int frame_count;
        double fps;
        int width;
        int height;
        bool has_audio;
    };

    VideoInfo get_video_info(const std::filesystem::path& video_path) {
        cv::VideoCapture cap(video_path.string());
        VideoInfo info;
        info.frame_count = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));
        info.fps = cap.get(cv::CAP_PROP_FPS);
        info.width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
        info.height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
        // has_audio 需要 FFmpeg 检查，OpenCV 无法直接准确检测，这里暂定为 true
        info.has_audio = true;
        return info;
    }
};

TEST_F(E2EVideoSwapTest, Video720pVertical_ProcessesWithCorrectFrameCount) {
    // Arrange
    auto output_path = output_dir_ / "result_slideshow.mp4";
    auto input_info = get_video_info(video_path_);

    config::TaskConfig task_config;
    task_config.task_info.id = "video_720p_vertical";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {video_path_.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.audio_policy = config::AudioPolicy::Copy;

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    // Act
    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration = duration_cast<seconds>(steady_clock::now() - start);

    // Assert
    ASSERT_TRUE(result.is_ok()) << "Pipeline failed: " << result.error().message;
    ASSERT_TRUE(std::filesystem::exists(output_path)) << "Output video not found";

    // 验证帧数
    auto output_info = get_video_info(output_path);
    // 允许少量帧误差 (例如 FFmpeg 处理时可能丢 1-2 帧)
    EXPECT_NEAR(output_info.frame_count, input_info.frame_count, 5)
        << "Frame count mismatch: expected " << input_info.frame_count << ", got "
        << output_info.frame_count;

    // 验证分辨率
    EXPECT_EQ(output_info.width, input_info.width);
    EXPECT_EQ(output_info.height, input_info.height);
}

TEST_F(E2EVideoSwapTest, Video720pVertical_AchievesMinimumFPS) {
    // Arrange
    auto output_path = output_dir_ / "result_slideshow_fps.mp4";
    auto input_info = get_video_info(video_path_);

    config::TaskConfig task_config;
    task_config.task_info.id = "video_720p_fps_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {video_path_.string()};
    task_config.io.output.path = output_dir_.string();

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    // Act
    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();

    // Assert
    ASSERT_TRUE(result.is_ok());

    // 计算实际 FPS
    double actual_fps = (input_info.frame_count * 1000.0) / duration_ms;

    std::cout << "=== Performance Summary ===" << std::endl;
    std::cout << "Total frames: " << input_info.frame_count << std::endl;
    std::cout << "Duration: " << duration_ms << " ms" << std::endl;
    std::cout << "Actual FPS: " << actual_fps << std::endl;

    // RTX 4060 8GB 标准: > 15 FPS

    // 如果是在 Debug 模式下运行，FPS 会很低，这里仅作记录，不强制失败
    // 或者我们应该根据构建模式调整期望？
    // 既然这是端到端测试，且用户可能在 Debug 下运行，我们降低一点期望或者仅打印

#ifdef NDEBUG
    constexpr double MIN_FPS_RTX4060 = 15.0;
    EXPECT_GE(actual_fps, MIN_FPS_RTX4060)
        << "FPS below threshold: " << actual_fps << " (min: " << MIN_FPS_RTX4060 << ")";
#else
    std::cout << "[WARN] Running in DEBUG mode. FPS requirement ignored. Got: " << actual_fps
              << std::endl;
#endif
}

TEST_F(E2EVideoSwapTest, Video720pVertical_CompletesWithinTimeLimit) {
    // Arrange
    config::TaskConfig task_config;
    task_config.task_info.id = "video_720p_time_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {video_path_.string()};
    task_config.io.output.path = output_dir_.string();

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    // Act
    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration_s = duration_cast<seconds>(steady_clock::now() - start).count();

    // Assert
    ASSERT_TRUE(result.is_ok());

    // RTX 4060 8GB 标准: < 40s (带 20% 余量)

#ifdef NDEBUG
    constexpr int64_t MAX_DURATION_SECONDS = 40;
    EXPECT_LT(duration_s, MAX_DURATION_SECONDS)
        << "Processing time exceeded: " << duration_s << "s (max: " << MAX_DURATION_SECONDS << "s)";
#else
    std::cout << "[WARN] Running in DEBUG mode. Time requirement ignored. Got: " << duration_s
              << "s" << std::endl;
#endif
}
