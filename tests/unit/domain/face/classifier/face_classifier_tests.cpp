#include <gtest/gtest.h>
#include <memory>
#include <opencv2/opencv.hpp>
#include <filesystem>

import domain.face.classifier;
import domain.face;
import domain.face.detector;
import domain.face.helper;
import foundation.infrastructure.test_support;
import domain.ai.model_manager;

using namespace domain::face::classifier;
using namespace domain::face;
using namespace domain::face::detector;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class FaceClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FaceClassifierTest, FactoryCreatesFairFace) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    EXPECT_NE(classifier, nullptr);
}

TEST_F(FaceClassifierTest, ClassifierNotLoadedInitially) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    ASSERT_NE(classifier, nullptr);
    // Classifier is created but model is not loaded yet
    // We can't call classify without loading a model first
}

TEST_F(FaceClassifierTest, ClassificationResultDefaultValues) {
    ClassificationResult result{};
    // Default zero-initialized values
    EXPECT_EQ(result.gender, Gender::Male); // 0
    EXPECT_EQ(result.race, Race::Black);    // 0
    EXPECT_EQ(result.age.min, 0);
    EXPECT_EQ(result.age.max, 100);
}

TEST(FaceClassifierIntegrationTest, ClassifyDetectedFace_Tiffany) {
    try {
        // 1. Configure ModelManager
        auto assets_path = get_assets_path();
        auto models_path = assets_path / "models_info.json";

        if (fs::exists(models_path)) {
            domain::ai::model_manager::ModelManager::get_instance()->set_model_info_file_path(
                models_path.string());
        } else {
            GTEST_SKIP() << "models_info.json not found at " << models_path;
        }

        auto model_manager = domain::ai::model_manager::ModelManager::get_instance();

        // 2. Load test image
        auto img_path = get_test_data_path("standard_face_test_iamges/tiffany.bmp");
        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        // 3. Detect face using face detector (auto-download if needed)
        std::string detector_model_key = "face_detector_yoloface";
        std::string detector_model_path = model_manager->ensure_model(detector_model_key);
        if (detector_model_path.empty()) {
            GTEST_SKIP() << "Model " << detector_model_key << " not available. Skipping test.";
        }

        auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
        ASSERT_NE(detector, nullptr);
        detector->load_model(detector_model_path);

        auto detections = detector->detect(frame);
        ASSERT_GE(detections.size(), 1) << "Should detect at least 1 face in tiffany.bmp";

        // 4. Get the first detected face's landmarks (5-point)
        auto& first_detection = detections[0];
        ASSERT_EQ(first_detection.landmarks.size(), 5)
            << "Expected 5-point landmarks from detector";

        // 5. Create and load face classifier (auto-download if needed)
        std::string classifier_model_key = "fairface";
        std::string classifier_model_path = model_manager->ensure_model(classifier_model_key);
        if (classifier_model_path.empty()) {
            GTEST_SKIP() << "Model " << classifier_model_key << " not available. Skipping test.";
        }

        auto classifier = create_classifier(ClassifierType::FairFace);
        ASSERT_NE(classifier, nullptr);
        classifier->load_model(classifier_model_path);

        // 6. Classify the face
        ClassificationResult result = classifier->classify(frame, first_detection.landmarks);

        // 7. Verify results - Tiffany is female, Asian
        EXPECT_EQ(result.gender, Gender::Female) << "Tiffany should be classified as Female";
        EXPECT_EQ(result.race, Race::Asian) << "Tiffany should be classified as Asian";

        // Age should be reasonable (Tiffany is typically in her 20s-30s)
        EXPECT_GE(result.age.min, 10) << "Age min should be reasonable";
        EXPECT_LE(result.age.max, 50) << "Age max should be reasonable";

        std::cout << "[INFO] Classification result: "
                  << "Gender=" << (result.gender == Gender::Female ? "Female" : "Male")
                  << ", Race=" << static_cast<int>(result.race) << ", Age=[" << result.age.min
                  << "-" << result.age.max << "]" << std::endl;

    } catch (const std::exception& e) { FAIL() << "Classification test failed: " << e.what(); }
}
