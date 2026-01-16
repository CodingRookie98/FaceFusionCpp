#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <filesystem>
#include <iostream>

import domain.face;
import domain.face.landmarker;
import domain.face.detector;
import domain.ai.model_repository;
import foundation.ai.inference_session;

class FaceLandmarkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        model_repo = domain::ai::model_repository::ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");

        std::string image_path = "./assets/standard_face_test_images/lenna.bmp";
        if (std::filesystem::exists(image_path)) { test_image = cv::imread(image_path); }
    }

    // 辅助方法：获取准确的 BBox (方案 B)
    cv::Rect2f get_face_bbox() {
        if (test_image.empty()) return cv::Rect2f();

        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::SCRFD);
        std::string model_path = model_repo->ensure_model("face_detector_scrfd");
        detector->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

        auto results = detector->detect(test_image);
        if (results.empty()) return cv::Rect2f();

        // DetectionResult 是 struct，直接访问 box 成员
        return results[0].box;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    cv::Mat test_image;
};

TEST_F(FaceLandmarkerTest, FactoryCreatesModels) {
    auto _2dfan = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::_2DFAN);
    EXPECT_NE(_2dfan, nullptr);

    auto peppawutz = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::Peppawutz);
    EXPECT_NE(peppawutz, nullptr);

    auto _68by5 = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::_68By5);
    EXPECT_NE(_68by5, nullptr);
}

TEST_F(FaceLandmarkerTest, _2DFAN_Inference) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    cv::Rect2f bbox = get_face_bbox();
    ASSERT_GT(bbox.width, 0);

    auto landmarker = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::_2DFAN);
    auto model_path = model_repo->ensure_model("face_landmarker_68");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    auto result = landmarker->detect(test_image, bbox);

    EXPECT_EQ(result.landmarks.size(), 68);
    EXPECT_GT(result.score, 0.5f);
}

TEST_F(FaceLandmarkerTest, Peppawutz_Inference) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    cv::Rect2f bbox = get_face_bbox();
    ASSERT_GT(bbox.width, 0);

    auto landmarker = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::Peppawutz);
    auto model_path = model_repo->ensure_model("face_landmarker_peppawutz");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    auto result = landmarker->detect(test_image, bbox);

    EXPECT_EQ(result.landmarks.size(), 68);
    EXPECT_GT(result.score, 0.3f);
}

TEST_F(FaceLandmarkerTest, _68By5_Inference) {
    auto landmarker = domain::face::landmarker::create_landmarker(
        domain::face::landmarker::LandmarkerType::_68By5);
    auto model_path = model_repo->ensure_model("face_landmarker_68_5");
    landmarker->load_model(model_path,
                           foundation::ai::inference_session::Options::with_best_providers());

    domain::face::types::Landmarks landmarks5 = {
        {100, 100}, {200, 100}, {150, 150}, {120, 200}, {180, 200}};

    auto landmarks68 = landmarker->expand_68_from_5(landmarks5);
    EXPECT_EQ(landmarks68.size(), 68);
}
