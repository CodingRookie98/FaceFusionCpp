/**
 * @file face_analyser_integration_tests.cpp
 * @brief Integration tests for FaceAnalyser using real models and images.
 * @author CodingRookie
 * @date 2026-01-31
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <filesystem>

import domain.face.analyser;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.store;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face;
using namespace domain::face::analyser;
using namespace domain::face::detector;
using namespace domain::face::landmarker;
using namespace domain::face::recognizer;
using namespace domain::face::classifier;
using namespace domain::face::store;
using namespace domain::ai::model_repository;

class FaceAnalyserIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        FaceStore::get_instance()->clear_faces();
        model_repo = ModelRepository::get_instance();
        model_repo->set_model_info_file_path("./assets/models_info.json");

        std::string image_path = "./assets/standard_face_test_images/lenna.bmp";
        if (std::filesystem::exists(image_path)) { test_image = cv::imread(image_path); }
    }

    std::shared_ptr<ModelRepository> model_repo;
    cv::Mat test_image;
};

TEST_F(FaceAnalyserIntegrationTest, RealImageE2ETest) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    Options real_options;
    real_options.model_paths.face_detector_scrfd = model_repo->ensure_model("scrfd");
    real_options.model_paths.face_landmarker_68by5 = model_repo->ensure_model("68_by_5");
    real_options.model_paths.face_recognizer_arcface =
        model_repo->ensure_model("arcface_w600k_r50");
    real_options.model_paths.face_classifier_fairface = model_repo->ensure_model("fairface");

    real_options.face_detector_options.type = DetectorType::SCRFD;
    real_options.face_landmarker_options.type = LandmarkerType::_68By5;
    real_options.inference_session_options =
        foundation::ai::inference_session::Options::with_best_providers();

    ASSERT_FALSE(real_options.model_paths.face_detector_scrfd.empty());

    FaceAnalyser analyser(real_options);
    auto faces = analyser.get_many_faces(test_image);

    ASSERT_FALSE(faces.empty()) << "Should detect at least one face in lenna.bmp";
    EXPECT_GT(faces[0].detector_score(), 0.5f);
    EXPECT_EQ(faces[0].kps().size(), 68); // 68by5 should result in 68 points
    EXPECT_FALSE(faces[0].embedding().empty());
}

TEST_F(FaceAnalyserIntegrationTest, ModelReuseTest) {
    if (test_image.empty()) GTEST_SKIP() << "Test image not found";

    Options opts;
    opts.model_paths.face_detector_scrfd = model_repo->ensure_model("scrfd");
    opts.face_detector_options.type = DetectorType::SCRFD;

    FaceAnalyser analyser1(opts);
    auto detector1 = analyser1.get_many_faces(test_image); // Trigger loading

    // Create a second analyser with same options
    FaceAnalyser analyser2(opts);

    // We can't directly access the private m_detector, but we can verify behavior
    // or use a friend class / test support if available.
    // For now, let's verify update_options with non-structural change.

    opts.face_detector_options.min_score = 0.6f;
    analyser1.update_options(opts);

    // If it didn't crash and still works, it's a good sign.
    auto faces = analyser1.get_many_faces(test_image);
    ASSERT_FALSE(faces.empty());
}
