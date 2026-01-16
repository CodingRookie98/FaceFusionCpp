#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.enhancer;
import domain.face.detector;
import domain.face; // Import base face module for types
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::enhancer;
using namespace foundation::infrastructure::test;

class FaceEnhancerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        repo = domain::ai::model_repository::ModelRepository::get_instance();
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        // Use standard test image
        target_path = get_test_data_path("standard_face_test_images/lenna.bmp");
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

        // Return the first face's landmarks
        return results[0].landmarks;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path target_path;
};

TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceWithCodeFormer) {
    if (!std::filesystem::exists(target_path)) {
        GTEST_SKIP() << "Test image not found: " << target_path;
    }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = get_face_landmarks(target_img);
    ASSERT_FALSE(target_kps.empty()) << "No face detected in target image";

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::CodeFormer);

    std::string model_path = repo->ensure_model("codeformer");
    ASSERT_FALSE(model_path.empty()) << "CodeFormer model not found";

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    EnhanceInput input;
    input.target_frame = target_img;
    input.target_faces_landmarks = {target_kps};
    input.face_blend = 100; // Full enhancement
    // input.mask_options default is Box, which is fine

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(input);
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    EXPECT_EQ(result_img.size(), target_img.size());
    EXPECT_EQ(result_img.type(), target_img.type());

    // Save result for visual inspection
    std::filesystem::create_directories("tests_output");
    cv::imwrite("tests_output/enhance_codeformer_result.jpg", result_img);
}

TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceWithGfpGan) {
    if (!std::filesystem::exists(target_path)) {
        GTEST_SKIP() << "Test image not found: " << target_path;
    }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = get_face_landmarks(target_img);
    ASSERT_FALSE(target_kps.empty()) << "No face detected in target image";

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::GfpGan);

    // Using gfpgan_1.4 as default test model
    std::string model_path = repo->ensure_model("gfpgan_1.4");
    ASSERT_FALSE(model_path.empty()) << "GFPGAN model not found";

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    EnhanceInput input;
    input.target_frame = target_img;
    input.target_faces_landmarks = {target_kps};
    input.face_blend = 100;

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(input);
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    EXPECT_EQ(result_img.size(), target_img.size());

    // Save result for visual inspection
    std::filesystem::create_directories("tests_output");
    cv::imwrite("tests_output/enhance_gfpgan_result.jpg", result_img);
}
