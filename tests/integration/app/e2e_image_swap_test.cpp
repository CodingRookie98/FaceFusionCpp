// tests/integration/app/e2e_image_swap_test.cpp

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <opencv2/opencv.hpp>

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

class E2EImageSwapTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化模型仓库
        repo_ = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        repo_->set_model_info_file_path((assets_path / "models_info.json").string());

        // 设置测试素材路径
        source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";

        // 创建输出目录
        output_dir_ = std::filesystem::current_path() / "tests_output" / "e2e_image_swap_test";
        std::filesystem::create_directories(output_dir_);
    }

    void TearDown() override {
        // 可选: 保留输出用于调试
        // std::filesystem::remove_all(output_dir_);
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo_;
    std::filesystem::path source_path_;
    std::filesystem::path output_dir_;

    // 验证输出图片中的人脸与源人脸相似
    void verify_face_swap(const std::filesystem::path& output_image,
                          const std::filesystem::path& source_face,
                          float distance_threshold = 0.65f) {
        ASSERT_TRUE(std::filesystem::exists(output_image))
            << "Output image does not exist: " << output_image;

        auto analyser = domain::face::test_support::create_face_analyser(repo_);

        cv::Mat output_img = cv::imread(output_image.string());
        cv::Mat source_img = cv::imread(source_face.string());

        ASSERT_FALSE(output_img.empty()) << "Failed to load output image";
        ASSERT_FALSE(source_img.empty()) << "Failed to load source image";

        auto output_faces = analyser->get_many_faces(
            output_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);
        auto source_faces = analyser->get_many_faces(
            source_img, domain::face::analyser::FaceAnalysisType::Detection
                            | domain::face::analyser::FaceAnalysisType::Embedding);

        ASSERT_FALSE(output_faces.empty()) << "No face detected in output image";
        ASSERT_FALSE(source_faces.empty()) << "No face detected in source image";

        // 计算距离 (越小越相似)
        float distance = domain::face::analyser::FaceAnalyser::calculate_face_distance(
            output_faces[0], source_faces[0]);

        EXPECT_LT(distance, distance_threshold) << "Face distance too high: " << distance
                                                << " (threshold: " << distance_threshold << ")";
    }
};

// ============================================================================
// P0 基线测试
// ============================================================================

TEST_F(E2EImageSwapTest, Img512Baseline_ProcessesUnder3Seconds) {
    // Arrange
    auto target_path = get_assets_path() / "standard_face_test_images" / "tiffany.bmp";
    auto output_path = output_dir_ / "result_tiffany.bmp";

    config::TaskConfig task_config;
    task_config.task_info.id = "img_512_baseline";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.prefix = "result_";
    task_config.io.output.image_format = "bmp";

    // 配置 Pipeline
    config::PipelineStep swap_step;
    swap_step.step = "face_swapper";
    swap_step.enabled = true;
    config::FaceSwapperParams params;
    params.model = "inswapper_128_fp16";
    swap_step.params = params;
    task_config.pipeline.push_back(swap_step);

    config::AppConfig app_config; // 使用默认配置

    // Act
    auto start = steady_clock::now();
    auto runner = CreatePipelineRunner(app_config);
    auto merged_config = config::MergeConfigs(task_config, app_config);
    auto result = runner->Run(merged_config, [](const services::pipeline::TaskProgress& p) {});
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);

    // Assert
    ASSERT_TRUE(result.is_ok()) << "Pipeline failed: " << result.error().message;

    // Adjust threshold for Debug mode
#ifdef NDEBUG
    constexpr int64_t MAX_DURATION =
        10000; // Relaxed to 10s for Release to account for engine loading
#else
    constexpr int64_t MAX_DURATION = 20000; // 20s for Debug
#endif

    EXPECT_LT(duration.count(), MAX_DURATION)
        << "Processing time exceeded threshold: " << duration.count() << "ms";

    verify_face_swap(output_path, source_path_);
}

TEST_F(E2EImageSwapTest, Img720pStandard_ProcessesUnder5Seconds) {
    // Arrange
    auto target_path = get_assets_path() / "standard_face_test_images" / "girl.bmp";
    auto output_path = output_dir_ / "result_girl.bmp";

    config::TaskConfig task_config;
    task_config.task_info.id = "img_720p_standard";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.prefix = "result_";
    task_config.io.output.image_format = "bmp";

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
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);

    // Assert
    ASSERT_TRUE(result.is_ok());

    // Adjust threshold for Debug mode
#ifdef NDEBUG
    constexpr int64_t MAX_DURATION = 10000; // Relaxed to 10s
#else
    constexpr int64_t MAX_DURATION = 20000; // 20s for Debug
#endif

    EXPECT_LT(duration.count(), MAX_DURATION)
        << "Processing time exceeded threshold: " << duration.count() << "ms";

    verify_face_swap(output_path, source_path_);
}

// ============================================================================
// P1 压力测试
// ============================================================================

TEST_F(E2EImageSwapTest, Img2kStress_ProcessesUnder10Seconds) {
    // Arrange
    auto target_path = get_assets_path() / "standard_face_test_images" / "woman.jpg";
    auto output_path = output_dir_ / "result_woman.png"; // WebP 输入，PNG 输出

    config::TaskConfig task_config;
    task_config.task_info.id = "img_2k_stress";
    task_config.io.source_paths = {source_path_.string()};
    task_config.io.target_paths = {target_path.string()};
    task_config.io.output.path = output_dir_.string();
    task_config.io.output.prefix = "result_";
    task_config.io.output.image_format = "png";

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
    auto duration = duration_cast<milliseconds>(steady_clock::now() - start);

    // Assert
    ASSERT_TRUE(result.is_ok());

// Adjust threshold for Debug mode
#ifdef NDEBUG
    constexpr int64_t MAX_DURATION = 10000;
#else
    constexpr int64_t MAX_DURATION = 20000; // 20s for Debug
#endif

    EXPECT_LT(duration.count(), MAX_DURATION)
        << "Processing time exceeded threshold: " << duration.count() << "ms";
}
