#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <filesystem>
#include <iostream>
#include <vector>

import domain.face.recognizer;
import domain.face.test_support;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;
import domain.face;

using namespace domain::face::recognizer;
using namespace domain::face::test_support;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class FaceRecognizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        model_repo = setup_model_repository(assets_path);
        test_image_path = get_test_data_path("standard_face_test_images/lenna.bmp");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    fs::path test_image_path;
};

TEST_F(FaceRecognizerTest, FactoryCreatesArcFace) {
    auto recognizer = domain::face::recognizer::create_face_recognizer(
        domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);
    EXPECT_NE(recognizer, nullptr);
}

TEST_F(FaceRecognizerTest, ArcFaceInference) {
    if (!fs::exists(test_image_path)) {
        GTEST_SKIP() << "Test image not found: " << test_image_path;
    }

    cv::Mat test_image = cv::imread(test_image_path.string());
    if (test_image.empty()) GTEST_SKIP() << "Failed to read test image";

    auto landmarks = detect_face_landmarks(test_image, model_repo);
    if (landmarks.empty()) GTEST_SKIP() << "Could not detect face for testing";

    auto recognizer = domain::face::recognizer::create_face_recognizer(
        domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);

    auto model_path = model_repo->ensure_model("arcface_w600k_r50");
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

// Force rebuild
