#include <gtest/gtest.h>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <regex>
#include <fstream>
#include <memory>

import services.pipeline.runner;
import config.task;
import config.app;
import config.merger;
import foundation.infrastructure.test_support;
import foundation.infrastructure.logger;
import foundation.media.vision;
import foundation.media.ffmpeg;
import domain.ai.model_repository;

using namespace services::pipeline;
using namespace foundation::infrastructure::test;

extern void LinkGlobalTestEnvironment();

class EdgeCasesTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        repo_ = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        repo_->set_model_info_file_path((assets_path / "models_info.json").string());

        source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";

        output_dir_ = std::filesystem::temp_directory_path() / "facefusion_tests" / "edge_cases";
        std::filesystem::create_directories(output_dir_);
    }

    void TearDown() override {
        if (std::filesystem::exists(output_dir_)) {
            std::error_code ec;
            std::filesystem::remove_all(output_dir_, ec);
        }
    }

    std::filesystem::path source_path_;
    std::filesystem::path output_dir_;
    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo_;
};

// ============================================================================
// Edge Case 1: Palette image (pal8) auto-conversion
// ============================================================================

TEST_F(EdgeCasesTest, PaletteImage_AutoConvertsToRGB24) {
    auto target_path = get_assets_path() / "standard_face_test_images" / "man.bmp";
    auto output_path = output_dir_ / "result_man.bmp";

    cv::Mat input = cv::imread(target_path.string(), cv::IMREAD_UNCHANGED);
    ASSERT_FALSE(input.empty()) << "Failed to load man.bmp";

    config::TaskConfig task_config;
    task_config.task_info.id = "palette_edge_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.image_format = "bmp";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config);

    if (!result.is_ok()) {
        std::cout << "Pipeline failed with error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok()) << "Pipeline should handle pal8 format";
    ASSERT_TRUE(std::filesystem::exists(output_path)) << "Output should be generated";

    cv::Mat output = cv::imread(output_path.string());
    EXPECT_EQ(output.channels(), 3) << "Output should be RGB (3 channels)";
    EXPECT_EQ(output.type(), CV_8UC3) << "Output should be 8-bit BGR";
}

// ============================================================================
// Edge Case 2: Format disguise (WebP with .jpg extension)
// ============================================================================

TEST_F(EdgeCasesTest, FormatDisguise_WebPWithJpgExtension_DecodesCorrectly) {
    auto target_path = get_assets_path() / "standard_face_test_images" / "woman.jpg";
    auto output_path = output_dir_ / "result_woman.png";

    {
        std::ifstream file(target_path, std::ios::binary);
        char magic[12];
        file.read(magic, 12);
        bool is_webp = (std::string(magic, 4) == "RIFF" && std::string(magic + 8, 4) == "WEBP");
        EXPECT_TRUE(is_webp) << "woman.jpg should actually be WebP format";
    }

    config::TaskConfig task_config;
    task_config.task_info.id = "format_disguise_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.image_format = "png";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config);

    if (!result.is_ok()) {
        std::cout << "Pipeline failed with error: " << result.error().message << std::endl;
    }
    ASSERT_TRUE(result.is_ok()) << "Pipeline should handle WebP disguised as JPG";
    ASSERT_TRUE(std::filesystem::exists(output_path));

    cv::Mat output = cv::imread(output_path.string());
    EXPECT_FALSE(output.empty()) << "Output image should be valid";
}

// ============================================================================
// Edge Case 3: No-face frame passthrough (E403)
// ============================================================================

class NoFaceFrameTest : public EdgeCasesTest {
protected:
    cv::Mat create_no_face_image(int width = 640, int height = 480) {
        cv::Mat img(height, width, CV_8UC3, cv::Scalar(100, 150, 200));
        cv::circle(img, cv::Point(width / 2, height / 2), 100, cv::Scalar(255, 0, 0), -1);
        cv::rectangle(img, cv::Point(50, 50), cv::Point(150, 150), cv::Scalar(0, 255, 0), -1);
        return img;
    }
};

TEST_F(NoFaceFrameTest, NoFaceDetected_PassthroughWithWarning) {
    auto no_face_img = create_no_face_image();
    auto target_path = output_dir_ / "no_face_input.bmp";
    cv::imwrite(target_path.string(), no_face_img);

    auto output_path = output_dir_ / "result_no_face_input.bmp";

    config::TaskConfig task_config;
    task_config.task_info.id = "no_face_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.image_format = "bmp";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config);

    EXPECT_TRUE(result.is_ok()) << "Pipeline should not fail on no-face images";
    EXPECT_TRUE(std::filesystem::exists(output_path)) << "Output should exist (passthrough)";

    cv::Mat output = cv::imread(output_path.string());
    EXPECT_FALSE(output.empty());
}

// ============================================================================
// Edge Case 4: Vertical video aspect ratio preservation
// ============================================================================

TEST_F(EdgeCasesTest, VerticalVideo_PreservesAspectRatio) {
    auto target_path = get_assets_path() / "standard_face_test_videos" / "slideshow_scaled.mp4";

    foundation::media::ffmpeg::VideoParams video_params(target_path.string());
    int orig_width = video_params.width;
    int orig_height = video_params.height;
    double orig_aspect = static_cast<double>(orig_width) / orig_height;

    ASSERT_LT(orig_aspect, 1.0) << "Test video should be vertical (portrait)";
    ASSERT_EQ(orig_width, 720);
    ASSERT_EQ(orig_height, 1280);

    auto output_path = output_dir_ / "result_slideshow_scaled.mp4";

    config::TaskConfig task_config;
    task_config.task_info.id = "vertical_video_test";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.image_format = "png";

    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams swap_params;
    swap_params.model = "inswapper_128_fp16";
    swap_step.params = swap_params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config;

    auto runner = create_pipeline_runner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->run(merged_config);

    ASSERT_TRUE(result.is_ok());
    ASSERT_TRUE(std::filesystem::exists(output_path));

    foundation::media::ffmpeg::VideoParams out_params(output_path.string());
    int out_width = out_params.width;
    int out_height = out_params.height;
    double out_aspect = static_cast<double>(out_width) / out_height;

    EXPECT_EQ(out_width, orig_width) << "Width should match";
    EXPECT_EQ(out_height, orig_height) << "Height should match";
    EXPECT_NEAR(out_aspect, orig_aspect, 0.01) << "Aspect ratio should be preserved";
}
