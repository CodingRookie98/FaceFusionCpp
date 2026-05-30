1 | /**
  2| * @file test_face_detector.cpp
  3| * @brief Integration tests for FaceDetector.
  4| * @author CodingRookie
  5| * * @date 2026-01-27
  6| */
    7 | 8 | #include < gtest / gtest.h > 9 | #include < opencv2 / opencv.hpp > 10
    | #include < filesystem > 11 | 12 | import domain.face.detector;
13 | import tests.helpers.foundation.test_utilities;
14 | import domain.ai.model_repository;
15 | import foundation.ai.inference_session;
16 | 17 | using namespace domain::face::detector;
18 | using namespace tests::helpers::foundation;
19 | using namespace foundation::ai::inference_session;
20 | namespace fs = std::filesystem;
21 | 22 | extern void LinkGlobalTestEnvironment();
23 | 24 | class FaceDetectorTest : public ::testing::Test {
    25 | protected : 26 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    27 | 28 | void SetUp() override {
        29 | // 1. Configure ModelRepository to find models
            30 | auto assets_path = get_assets_path();
        31 | auto models_info_path = assets_path / "models_info.json";
        32 | 33 | if (fs::exists(models_info_path)) {
            34
                | domain::ai::model_repository::ModelRepository::get_instance()
                      ->set_model_info_file_path(35 | models_info_path.string());
            36 |
        }
        37 |
    }
    38 |
};
39 | 40 | TEST_F(FaceDetectorTest, DetectFacesTiffanyImageFindsAtLeastOneFace) {
    41 | try {
        42 | auto model_repository = domain::ai::model_repository::ModelRepository::get_instance();
        43 | auto img_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
        44 | if (!fs::exists(img_path)) {
            GTEST_SKIP() << "Test image not found: " << img_path;
        }
        45 | 46 | cv::Mat frame = cv::imread(img_path.string());
        47 | ASSERT_FALSE(frame.empty()) << "Failed to read image: " << img_path;
        48 | 49 | auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
        50 | ASSERT_NE(detector, nullptr);
        51 | 52 | std::string model_key = "yoloface";
        53 | std::string model_path = model_repository->ensure_model(model_key);
        54 | if (model_path.empty()) {
            55 | GTEST_SKIP() << "Model " << model_key << " not available. Skipping test.";
            56 |
        }
        57 | 58 | detector->load_model(model_path, Options::with_best_providers());
        59 | 60 | // 4. Detect
            61 | auto faces = detector->detect(frame);
        62 | 63 | // 5. Verify
            64 |  // Tiffany image usually contains 1 face
            65 | EXPECT_GE(faces.size(), 1) << "Should detect at least 1 face in tiffany.bmp";
        66 | 67 | if (!faces.empty()) {
            68 | EXPECT_GT(faces[0].score, 0.5f);
            69 | EXPECT_GT(faces[0].box.area(), 0);
            70 |
        }
        71 | 72 |
    } catch (const std::exception& e) {
        73 |     // If model loading fails (e.g. onnx file missing), we might catch it here
            74 | // We should fail or skip depending on policy.
            75 | // Since we want to enforce quality, let's Fail but provide info.
            76 | FAIL() << "Detection test failed: " << e.what();
        77 |
    }
    78 |
}
79 |