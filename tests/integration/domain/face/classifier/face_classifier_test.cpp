/**
 * @file face_classifier_tests.cpp
 * @brief Unit tests for FaceClassifier.
 * @author
 * CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <memory>
#include <opencv2/opencv.hpp>
#include <filesystem>

import domain.face.classifier;
import domain.face;
import domain.face.detector;
import domain.face.helper;
import tests.helpers.foundation.test_utilities;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face::classifier;
using namespace domain::face;
using namespace domain::face::detector;
using namespace tests::helpers::foundation;
using namespace foundation::ai::inference_session;
namespace fs = std::filesystem;

extern void LinkGlobalTestEnvironment();

class FaceClassifierTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {}
};

TEST_F(FaceClassifierTest, FactoryCreatesFairFace) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    ASSERT_NE(classifier, nullptr);
}

TEST_F(FaceClassifierTest, ClassificationResultDefaultValues) {
    ClassificationResult result{};
    EXPECT_EQ(result.gender, Gender::Male); // 0
    EXPECT_EQ(result.race, Race::Black);    // 0
    EXPECT_EQ(result.age.min, 0);
    EXPECT_EQ(result.age.max, 100);
}

TEST_F(FaceClassifierTest, ClassifyDetectedFaceTiffany) {
    try {
        // 1. Configure ModelRepository
        auto assets_path = get_assets_path();
        auto models_path = assets_path / "models_info.json";

        auto model_repository = domain::ai::model_repository::ModelRepository::get_instance();
        if (fs::exists(models_path)) {
            model_repository->set_model_info_file_path(models_path.string());
        } else {
            GTEST_SKIP() << "models_info.json not found at " << models_path;
        }

        // 2. Load test image
        auto img_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        // 3. Detect face using face detector
        // Use best available execution providers (TensorRT > CUDA > CPU)
        auto detector_options = Options::with_best_providers();

        std::string detector_model_key = "yoloface";
        std::string detector_model_path = model_repository->ensure_model(detector_model_key);
        if (detector_model_path.empty()) {
            GTEST_SKIP() << "Model " << detector_model_key << " not available.";
        }

        auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
        ASSERT_NE(detector, nullptr);
        detector->load_model(detector_model_path, detector_options);

        auto detections = detector->detect(frame);
        ASSERT_GE(detections.size(), 1);

        // 4. Get the first detected face's landmarks
        auto& first_detection = detections[0];

        // 5. Create and load face classifier
        // Use best available execution providers (TensorRT > CUDA > CPU)
        auto classifier_options = Options::with_best_providers();

        std::string classifier_model_key = "fairface";
        std::string classifier_model_path = model_repository->ensure_model(classifier_model_key);

        auto classifier = create_classifier(ClassifierType::FairFace);
        ASSERT_NE(classifier, nullptr);
        classifier->load_model(classifier_model_path, classifier_options);

        // 6. Classify the face
        ClassificationResult result = classifier->classify(frame, first_detection.landmarks);

        // 7. Verify results
        EXPECT_TRUE(result.gender == Gender::Male || result.gender == Gender::Female);
        EXPECT_GE(result.age.min, 0);
        EXPECT_LE(result.age.max, 100);

        std::cout << "[INFO] Classification result: "
                  << "Gender=" << (result.gender == Gender::Female ? "Female" : "Male")
                  << ", Race=" << static_cast<int>(result.race) << ", Age=[" << result.age.min
                  << "-" << result.age.max << "]" << std::endl;

    } catch (const std::exception& e) { FAIL() << "Test failed: " << e.what(); }
}
