#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.expression;
import domain.face.detector;
import domain.face; // for types
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::expression;
using namespace foundation::infrastructure::test;

class LivePortraitTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        repo = domain::ai::model_repository::ModelRepository::get_instance();
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        target_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
    }

    // Helper: Detect face landmarks (5 points)
    domain::face::types::Landmarks get_face_landmarks(const cv::Mat& image) {
        if (image.empty()) return {};

        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::Yolo);

        std::string model_path = repo->ensure_model("face_detector_yoloface");
        if (model_path.empty()) return {};

        detector->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

        auto results = detector->detect(image);
        if (results.empty()) return {};

        return results[0].landmarks;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
};

TEST_F(LivePortraitTest, Construction) {
    EXPECT_NO_THROW({
        auto restorer = create_live_portrait_restorer();
        EXPECT_NE(restorer, nullptr);
    });
}

TEST_F(LivePortraitTest, RestoreExpressionBasic) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_path)) {
        GTEST_SKIP() << "Test images not found";
    }

    cv::Mat source_img = cv::imread(source_path.string());
    cv::Mat target_img = cv::imread(target_path.string());

    ASSERT_FALSE(source_img.empty());
    ASSERT_FALSE(target_img.empty());

    // 1. Detect Landmarks
    auto source_kps = get_face_landmarks(source_img);
    auto target_kps = get_face_landmarks(target_img);

    ASSERT_FALSE(source_kps.empty()) << "No face detected in source";
    ASSERT_FALSE(target_kps.empty()) << "No face detected in target";

    // 2. Create Restorer
    auto restorer = create_live_portrait_restorer();

    // 3. Load Models
    // Keys from models_info.json or defaults
    std::string feature_path = repo->ensure_model("live_portrait_feature_extractor");
    std::string motion_path = repo->ensure_model("live_portrait_motion_extractor");
    std::string generator_path = repo->ensure_model("live_portrait_generator");

    // If models are not available, skip test but verify we handled it gracefully
    if (feature_path.empty() || motion_path.empty() || generator_path.empty()) {
        std::cout << "LivePortrait models not found in assets/models/, skipping inference test."
                  << std::endl;
        std::cout << "feature_path: " << feature_path << std::endl;
        std::cout << "motion_path: " << motion_path << std::endl;
        std::cout << "generator_path: " << generator_path << std::endl;
        GTEST_SKIP() << "LivePortrait models not found";
    }

    EXPECT_NO_THROW(
        restorer->load_model(feature_path, motion_path, generator_path,
                             foundation::ai::inference_session::Options::with_best_providers()));

    // 4. Prepare Input
    RestoreExpressionInput input;
    input.source_frame = source_img;
    input.source_landmarks = {source_kps};
    input.target_frame = target_img;
    input.target_landmarks = {target_kps};
    input.restore_factor = 0.5f;

    // 5. Run Restoration
    cv::Mat result;
    EXPECT_NO_THROW(result = restorer->restore_expression(input));

    ASSERT_FALSE(result.empty());
    EXPECT_EQ(result.size(), target_img.size());

    // Save output
    std::filesystem::create_directories("tests_output");
    cv::imwrite("tests_output/live_portrait_result.jpg", result);
}
