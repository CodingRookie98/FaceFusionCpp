/**
 * @file frame_enhancer_tests.cpp
 * @brief Unit tests for FrameEnhancer.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.frame.enhancer;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::frame::enhancer;
using namespace foundation::infrastructure::test;

extern void LinkGlobalTestEnvironment();

class FrameEnhancerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        repo = domain::ai::model_repository::ModelRepository::get_instance();
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
};

TEST_F(FrameEnhancerTest, ConstructionRealEsrGan) {
    EXPECT_NO_THROW({
        auto enhancer = FrameEnhancerFactory::create(
            FrameEnhancerType::RealEsrGan, "real_esrgan_x2_fp16",
            foundation::ai::inference_session::Options::with_best_providers());
        EXPECT_NE(enhancer, nullptr);
    });
}

TEST_F(FrameEnhancerTest, EnhanceFrameRealEsrGan) {
    if (!std::filesystem::exists(source_path)) { GTEST_SKIP() << "Test image not found"; }

    cv::Mat source_img = cv::imread(source_path.string());
    ASSERT_FALSE(source_img.empty());

    // Prepare Model
    std::string model_key = "real_esrgan_x2_fp16";
    // Ensure model is available (downloads if needed, or checks path)
    // Factory also does ensure_model, but we want to check availability to SKIP if needed.
    // If we are offline or model missing, ensure_model might fail or return empty path depending on
    // implementation. ModelRepository::ensure_model returns path.

    // NOTE: ModelRepository::ensure_model might download. For unit tests, we prefer pre-existing
    // models. However, the test environment should have them. If not, we skip. Since we don't have
    // a direct "check if exists without download" public API visible here (except getting path and
    // checking fs), we try to ensure.

    std::string model_path = repo->get_model_path(model_key);
    if (!std::filesystem::exists(model_path)) {
        std::cout << "Model " << model_key << " not found at " << model_path << ", skipping test."
                  << std::endl;
        GTEST_SKIP() << "Model file not found";
    }

    auto enhancer = FrameEnhancerFactory::create(
        FrameEnhancerType::RealEsrGan, model_key,
        foundation::ai::inference_session::Options::with_best_providers());

    ASSERT_NE(enhancer, nullptr);

    FrameEnhancerInput input;
    input.target_frame = source_img;
    input.blend = 100;

    cv::Mat result;
    EXPECT_NO_THROW(result = enhancer->enhance_frame(input));

    ASSERT_FALSE(result.empty());
    // x2 scale
    EXPECT_EQ(result.cols, source_img.cols * 2);
    EXPECT_EQ(result.rows, source_img.rows * 2);

    // Save output
    std::filesystem::path output_dir = "tests_output";
    if (!std::filesystem::exists(output_dir)) { std::filesystem::create_directories(output_dir); }
    std::string output_filename = "frame_enhancer_result_" + model_key + ".jpg";
    cv::imwrite((output_dir / output_filename).string(), result);
}
