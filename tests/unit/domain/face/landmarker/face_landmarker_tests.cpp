/**
 * @file face_landmarker_tests.cpp
 * @brief Unit tests for FaceLandmarker.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <filesystem>
#include <iostream>

import domain.face;
import domain.face.landmarker;
import domain.face.test_support;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face;
using namespace domain::face::landmarker;
using namespace domain::face::test_support;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class FaceLandmarkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        model_repo = setup_model_repository(assets_path);
        test_image_path = get_test_data_path("standard_face_test_images/lenna.bmp");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    fs::path test_image_path;
};

TEST_F(FaceLandmarkerTest, FactoryCreatesModels) {
    auto _2dfan = create_landmarker(LandmarkerType::_2DFAN);
    EXPECT_NE(_2dfan, nullptr);

    auto peppawutz = create_landmarker(LandmarkerType::Peppawutz);
    EXPECT_NE(peppawutz, nullptr);

    auto _68by5 = create_landmarker(LandmarkerType::_68By5);
    EXPECT_NE(_68by5, nullptr);
}

TEST_F(FaceLandmarkerTest, _2DFAN_Inference) {
    if (!fs::exists(test_image_path)) { GTEST_SKIP() << "Test image not found"; }

    cv::Mat test_image = cv::imread(test_image_path.string());
    if (test_image.empty()) GTEST_SKIP() << "Failed to load test image";

    cv::Rect2f bbox = detect_face_bbox(test_image, model_repo);
    if (bbox.width <= 0) { GTEST_SKIP() << "No face detected for testing"; }

    auto landmarker = create_landmarker(LandmarkerType::_2DFAN);
    auto model_path = model_repo->ensure_model("2dfan4");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    auto result = landmarker->detect(test_image, bbox);

    EXPECT_EQ(result.landmarks.size(), 68);
    EXPECT_GT(result.score, 0.5f);
}

TEST_F(FaceLandmarkerTest, Peppawutz_Inference) {
    if (!fs::exists(test_image_path)) { GTEST_SKIP() << "Test image not found"; }

    cv::Mat test_image = cv::imread(test_image_path.string());
    if (test_image.empty()) GTEST_SKIP() << "Failed to load test image";

    cv::Rect2f bbox = detect_face_bbox(test_image, model_repo);
    if (bbox.width <= 0) { GTEST_SKIP() << "No face detected for testing"; }

    auto landmarker = create_landmarker(LandmarkerType::Peppawutz);
    auto model_path = model_repo->ensure_model("peppawutz");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    auto result = landmarker->detect(test_image, bbox);

    EXPECT_EQ(result.landmarks.size(), 68);
    EXPECT_GT(result.score, 0.3f);
}

TEST_F(FaceLandmarkerTest, _68By5_Inference) {
    auto landmarker = create_landmarker(LandmarkerType::_68By5);
    auto model_path = model_repo->ensure_model("68_by_5");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    domain::face::types::Landmarks landmarks5 = {
        {100, 100}, {200, 100}, {150, 150}, {120, 200}, {180, 200}};

    auto landmarks68 = landmarker->expand_68_from_5(landmarks5);
    EXPECT_EQ(landmarks68.size(), 68);
}
