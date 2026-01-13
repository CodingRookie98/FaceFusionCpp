#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <filesystem>

import domain.face.detector;
import foundation.infrastructure.test_support;
import domain.ai.model_manager;

using namespace domain::face::detector;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

TEST(FaceDetectorFactoryTest, CreateYolo) {
    auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
    EXPECT_NE(detector, nullptr);
}

TEST(FaceDetectorFactoryTest, CreateOthers) {
    auto detector_scrfd = FaceDetectorFactory::create(DetectorType::SCRFD);
    EXPECT_NE(detector_scrfd, nullptr);

    auto detector_retina = FaceDetectorFactory::create(DetectorType::RetinaFace);
    EXPECT_NE(detector_retina, nullptr);
}

TEST(FaceDetectorTest, DetectFaces_Tiffany) {
    try {
        // 1. Configure ModelManager to find models
        auto assets_path = get_assets_path();
        auto models_path = assets_path / "models_info.json";

        if (fs::exists(models_path)) {
            // We assume ModelManager is a singleton and we can configure it
            // Note: We need to ensure domain.ai.model_manager is imported
            domain::ai::model_manager::ModelManager::get_instance()->set_model_info_file_path(
                models_path.string());
        } else {
            std::cout << "[WARNING] models_info.json not found at " << models_path << std::endl;
        }

        // 2. Load Image
        auto img_path = get_test_data_path("standard_face_test_iamges/tiffany.bmp");
        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        // 3. Create Detector
        // Note: Creation might fail or throw if model files listed in json are missing on disk
        auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
        ASSERT_NE(detector, nullptr);

        // 3.1 Load Model
        // We need to load the model before detection
        auto model_manager = domain::ai::model_manager::ModelManager::get_instance();
        std::string model_key = "face_detector_yoloface";

        // Ensure path is set (redundant if set above, but safe)
        if (model_manager->get_model_json_file_path().empty()) {
            model_manager->set_model_info_file_path((assets_path / "models_info.json").string());
        }

        if (!model_manager->is_downloaded(model_key)) {
            GTEST_SKIP() << "Model " << model_key << " not downloaded. Skipping test.";
        }

        std::string model_path = model_manager->get_model_path(model_key);
        // If absolute path needed:
        // model_path = (assets_path / "models" / "yoloface_8n.onnx").string();
        // But get_model_path should return valid path if is_downloaded is true.

        detector->load_model(model_path);

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
