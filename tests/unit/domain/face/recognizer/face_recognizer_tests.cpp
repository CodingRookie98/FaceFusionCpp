#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <filesystem>
#include <iostream>
#include <vector>

import domain.face.recognizer;
import domain.face.detector;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import domain.face;

class FaceRecognizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");

        std::string image_path = "./assets/standard_face_test_images/lenna.bmp";
        if (std::filesystem::exists(image_path)) { test_image = cv::imread(image_path); }
    }

    // Helper to get face landmarks (5 points)
    domain::face::types::Landmarks get_face_landmarks() {
        if (test_image.empty()) return {};

        // Use YoloFace or SCRFD as they output 5 points
        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::SCRFD);

        std::string model_path = model_repo->ensure_model("face_detector_scrfd");
        if (model_path.empty()) return {};

        detector->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

        auto results = detector->detect(test_image);
        if (results.empty()) return {};

        return results[0].landmarks;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    cv::Mat test_image;
};

TEST_F(FaceRecognizerTest, FactoryCreatesArcFace) {
    auto recognizer = domain::face::recognizer::create_face_recognizer(
        domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);
    EXPECT_NE(recognizer, nullptr);
}

TEST_F(FaceRecognizerTest, ArcFaceInference) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    auto landmarks = get_face_landmarks();
    if (landmarks.empty()) GTEST_SKIP() << "Could not detect face for testing";

    auto recognizer = domain::face::recognizer::create_face_recognizer(
        domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);

    auto model_path = model_repo->ensure_model("face_recognizer_arcface_w600k_r50");
    ASSERT_FALSE(model_path.empty()) << "Model not found";

    recognizer->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    auto result = recognizer->recognize(test_image, landmarks);

    // Check embedding size
    EXPECT_EQ(result.first.size(), 512);
    EXPECT_EQ(result.second.size(), 512);

    // Check normalization
    double norm = cv::norm(result.second, cv::NORM_L2);
    EXPECT_NEAR(norm, 1.0, 1e-5);

    // Check if raw embedding is not zero
    double raw_norm = cv::norm(result.first, cv::NORM_L2);
    EXPECT_GT(raw_norm, 0.0);
}
