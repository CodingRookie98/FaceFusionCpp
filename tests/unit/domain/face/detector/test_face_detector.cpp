#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <filesystem>

import domain.face.detector;
import foundation.infrastructure.test_support;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face::detector;
using namespace foundation::infrastructure::test;
using namespace foundation::ai::inference_session;
namespace fs = std::filesystem;

class FaceDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 1. Configure ModelRepository to find models
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        if (fs::exists(models_info_path)) {
            domain::ai::model_repository::ModelRepository::get_instance()->set_model_info_file_path(
                models_info_path.string());
        }
    }
};

TEST_F(FaceDetectorTest, DetectFaces_Tiffany) {
    try {
        auto model_repository = domain::ai::model_repository::ModelRepository::get_instance();
        auto img_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
        ASSERT_NE(detector, nullptr);

        std::string model_key = "yoloface";
        std::string model_path = model_repository->ensure_model(model_key);
        if (model_path.empty()) {
            GTEST_SKIP() << "Model " << model_key << " not available. Skipping test.";
        }

        detector->load_model(model_path, Options::with_best_providers());

        // 4. Detect
        // Interface uses 'detect' and returns 'DetectionResults'
        auto faces = detector->detect(frame);

        // 5. Verify
        // Tiffany image usually contains 1 face
        EXPECT_GE(faces.size(), 1) << "Should detect at least 1 face in tiffany.bmp";

        if (!faces.empty()) {
            EXPECT_GT(faces[0].score, 0.5f);
            EXPECT_GT(faces[0].box.area(), 0);
        }

    } catch (const std::exception& e) {
        // If model loading fails (e.g. onnx file missing), we might catch it here
        // We should fail or skip depending on policy.
        // Since we want to enforce quality, let's Fail but provide info.
        FAIL() << "Detection test failed: " << e.what();
    }
}
