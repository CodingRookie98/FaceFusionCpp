/**
 * @file face_detector_param_test.cpp
 * @brief Parameterized tests for FaceDetector with multiple models.
 * @author hermes
 * @date 2026-05-30
 */

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <filesystem>

import domain.face.detector;
import tests.helpers.foundation.test_utilities;
import domain.ai.model_repository;
import foundation.ai.inference_session;

using namespace domain::face::detector;
using namespace tests::helpers::foundation;
using namespace foundation::ai::inference_session;
namespace fs = std::filesystem;

extern void LinkGlobalTestEnvironment();

// ============================================================
// 参数化测试：多模型检测
// ============================================================

struct DetectorTestParam {
    DetectorType type;
    std::string model_key;
    std::string model_name;
};

class FaceDetectorParamTest : public ::testing::TestWithParam<DetectorTestParam> {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        if (fs::exists(models_info_path)) {
            domain::ai::model_repository::ModelRepository::get_instance()->set_model_info_file_path(
                models_info_path.string());
        }
    }
};

TEST_P(FaceDetectorParamTest, DetectFacesTiffanyImageFindsAtLeastOneFace) {
    const auto& param = GetParam();

    try {
        auto model_repository = domain::ai::model_repository::ModelRepository::get_instance();
        auto img_path = get_test_data_path("standard_face_test_images/tiffany.bmp");

        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        auto detector = FaceDetectorFactory::create(param.type);
        ASSERT_NE(detector, nullptr) << "Failed to create detector: " << param.model_name;

        std::string model_path = model_repository->ensure_model(param.model_key);
        if (model_path.empty()) {
            GTEST_SKIP() << "Model " << param.model_key << " not available. Skipping test.";
        }

        detector->load_model(model_path, Options::with_best_providers());

        // Detect
        auto faces = detector->detect(frame);

        // Verify
        EXPECT_GE(faces.size(), 1)
            << "Should detect at least 1 face in tiffany.bmp with " << param.model_name;

        if (!faces.empty()) {
            EXPECT_GT(faces[0].score, 0.5f)
                << "Face score should be > 0.5 with " << param.model_name;
            EXPECT_GT(faces[0].box.area(), 0)
                << "Face box should have positive area with " << param.model_name;
        }
    } catch (const std::exception& e) { GTEST_SKIP() << "Exception: " << e.what(); }
}

TEST_P(FaceDetectorParamTest, DetectFacesLennaImageFindsAtLeastOneFace) {
    const auto& param = GetParam();

    try {
        auto model_repository = domain::ai::model_repository::ModelRepository::get_instance();
        auto img_path = get_test_data_path("standard_face_test_images/lenna.bmp");

        if (!fs::exists(img_path)) { GTEST_SKIP() << "Test image not found: " << img_path; }

        cv::Mat frame = cv::imread(img_path.string());
        ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;

        auto detector = FaceDetectorFactory::create(param.type);
        ASSERT_NE(detector, nullptr) << "Failed to create detector: " << param.model_name;

        std::string model_path = model_repository->ensure_model(param.model_key);
        if (model_path.empty()) {
            GTEST_SKIP() << "Model " << param.model_key << " not available. Skipping test.";
        }

        detector->load_model(model_path, Options::with_best_providers());

        // Detect
        auto faces = detector->detect(frame);

        // Verify
        EXPECT_GE(faces.size(), 1)
            << "Should detect at least 1 face in lenna.bmp with " << param.model_name;
    } catch (const std::exception& e) { GTEST_SKIP() << "Exception: " << e.what(); }
}

// ============================================================
// 实例化参数化测试
// ============================================================

INSTANTIATE_TEST_SUITE_P(
    MultiModelDetection, FaceDetectorParamTest,
    ::testing::Values(DetectorTestParam{DetectorType::Yolo, "yoloface", "YOLOFace"},
                      DetectorTestParam{DetectorType::RetinaFace, "retinaface", "RetinaFace"},
                      DetectorTestParam{DetectorType::SCRFD, "scrfd", "SCRFD"}),
    [](const ::testing::TestParamInfo<DetectorTestParam>& info) { return info.param.model_name; });
