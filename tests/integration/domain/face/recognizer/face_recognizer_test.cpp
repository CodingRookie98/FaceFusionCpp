1 | /**
  2| * @file face_recognizer_tests.cpp
  3| * @brief Integration tests for FaceRecognizer.
  4| * @author
  5| * CodingRookie
  6| * @date 2026-01-27
  7| */
    8 | 9 | #include < gtest / gtest.h > 10 | #include < opencv2 / opencv.hpp > 11
    | #include < memory > 12 | #include < filesystem > 13 | #include < iostream > 14
    | #include < vector > 15 | 16 | import domain.face.recognizer;
17 | import tests.helpers.domain.face_test_helpers;
18 | import domain.ai.model_repository;
19 | import foundation.ai.inference_session;
20 | import tests.helpers.foundation.test_utilities;
21 | import domain.face;
22 | 23 | using namespace domain::face::recognizer;
24 | using namespace tests::helpers::domain;
25 | using namespace tests::helpers::foundation;
26 | namespace fs = std::filesystem;
27 | 28 | extern void LinkGlobalTestEnvironment();
29 | 30 | class FaceRecognizerTest : public ::testing::Test {
    31 | protected : 32 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    33 | 34 | void SetUp() override {
        35 | auto assets_path = get_assets_path();
        36 | model_repo = tests::helpers::domain::setup_model_repository(assets_path);
        37 | test_image_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        38 |
    }
    39 | 40 | std::shared_ptr<domain::ai::model_repository::ModelRepository> model_repo;
    41 | fs::path test_image_path;
    42 |
};
43 | 44 | TEST_F(FaceRecognizerTest, CreateRecognizerArcFaceTypeReturnsValidInstance) {
    45 | auto recognizer = domain::face::recognizer::create_face_recognizer(
        46 | domain::face::recognizer::FaceRecognizerType::ArcFaceW600kR50);
    47 | EXPECT_NE(recognizer, nullptr);
    48 |
}
49 | 50 | TEST_F(FaceRecognizerTest, RecognizeFaceValidInputReturnsNormalizedEmbedding) {
    51 | if (!fs::exists(test_image_path)) {
        52 | GTEST_SKIP() << "Test image not found: " << test_image_path;
        53 |
    }
    54 | 55 | cv::Mat test_image = cv::imread(test_image_path.string());
    56 | if (test_image.empty()) GTEST_SKIP() << "Failed to read test image";
    57 | 58 | auto landmarks =
        tests::helpers::domain::detect_face_landmarks(test_image, model_repo);
    59 | if (landmarks.empty()) GTEST_SKIP() << "Could not detect face for testing";
    60 | 61 | auto recognizer = domain::face::recognizer::create_face_recognizer(
        62 | domain::face::recognizer::FaceRecognizerType::ArcFaceW600kR50);
    63 | 64 | auto model_path = model_repo->ensure_model("arcface_w600k_r50");
    65 | ASSERT_FALSE(model_path.empty()) << "Model not found";
    66 | 67
        | recognizer->load_model(
            model_path, 68 | foundation::ai::inference_session::Options::with_best_providers());
    69 | 70 | auto result = recognizer->recognize(test_image, landmarks);
    71 | 72 | // Check embedding size
        73 | EXPECT_EQ(result.first.size(), 512);
    74 | EXPECT_EQ(result.second.size(), 512);
    75 | 76 | // Check normalization
        77 | double norm = cv::norm(result.second, cv::NORM_L2);
    78 | EXPECT_NEAR(norm, 1.0, 1e-5);
    79 | 80 | // Check if raw embedding is not zero
        81 | double raw_norm = cv::norm(result.first, cv::NORM_L2);
    82 | EXPECT_GT(raw_norm, 0.0);
    83 |
}
84 |