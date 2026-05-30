1 | /**
  2| * @file face_masker_tests.cpp
  3| * @brief Integration tests for FaceMasker.
  4| * @author CodingRookie
  5| *
  6| * @date 2026-01-27
  7| */
    8 | 9 | #include < gtest / gtest.h > 10 | #include < opencv2 / core.hpp > 11
    | #include < opencv2 / imgcodecs.hpp > 12 | #include < opencv2 / imgproc.hpp > 13
    | #include < unordered_set > 14 | #include < filesystem > 15 | #include < iostream > 16 | 17
    | import domain.face.masker;
18 | import tests.helpers.domain.face_test_helpers;
19 | import domain.ai.model_repository;
20 | import foundation.ai.inference_session;
21 | import tests.helpers.foundation.test_utilities;
22 | #include "common/test_paths.h" 23 | 24 | using namespace domain::face::masker;
25 | using namespace tests::helpers::domain;
26 | using namespace tests::helpers::foundation;
27 | namespace fs = std::filesystem;
28 | 29 | extern void LinkGlobalTestEnvironment();
30 | 31 | class FaceMaskerTest : public ::testing::Test {
    32 | protected : 33 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    34 | 35 | void SetUp() override {
        36 | auto assets_path = get_assets_path();
        37 | repo = tests::helpers::domain::setup_model_repository(assets_path);
        38 | test_image_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        39 | output_dir = tests::common::TestPaths::GetTestOutputDir("face_masker");
        40 |
    }
    41 | 42 | std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    43 | fs::path test_image_path;
    44 | fs::path output_dir;
    45 |
};
46 | 47 | // ============================================================================
    48 |  // Factory Exception Tests
    49 |  // ============================================================================
    50 | 51 | TEST_F(FaceMaskerTest, CreateOcclusionMaskerEmptyPathThrowsException) {
    52 | EXPECT_ANY_THROW(create_occlusion_masker(""));
    53 |
}
54 | 55 | TEST_F(FaceMaskerTest, CreateRegionMaskerEmptyPathThrowsException) {
    56 | EXPECT_ANY_THROW(create_region_masker(""));
    57 |
}
58 | 59 | TEST_F(FaceMaskerTest, CreateOcclusionMaskerInvalidPathThrowsException) {
    60 | EXPECT_ANY_THROW(create_occlusion_masker("invalid_path.onnx"));
    61 |
}
62 | 63 | TEST_F(FaceMaskerTest, CreateRegionMaskerInvalidPathThrowsException) {
    64 | EXPECT_ANY_THROW(create_region_masker("invalid_path.onnx"));
    65 |
}
66 | 67 | // ============================================================================
    68 |  // Occlusion Masker Integration Tests
    69 |  // ============================================================================
    70 | 71 | TEST_F(FaceMaskerTest, CreateOcclusionMaskValidInputReturnsValidMask) {
    72 | // Get model path
        73 | std::string model_path = repo->ensure_model("xseg_1");
    74 | if (model_path.empty()) {
        GTEST_SKIP() << "face_occluder model not available";
    }
    75 | 76 | // Load test image
        77 | if (!fs::exists(test_image_path)) {
        78 | GTEST_SKIP() << "Test image not found: " << test_image_path;
        79 |
    }
    80 | cv::Mat image = cv::imread(test_image_path.string());
    81 | ASSERT_FALSE(image.empty()) << "Failed to load test image";
    82 | 83 | // Detect face and get landmarks
        84 | auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    85 | if (landmarks.empty()) {
        GTEST_SKIP() << "No face detected in test image";
    }
    86 | 87 | // Create a face crop (simplified - using center crop for testing)
        88 |  // In real usage, the face would be aligned using landmarks
        89 | int crop_size = 256;
    90 | int cx = static_cast<int>(landmarks[2].x); // nose point
    91 | int cy = static_cast<int>(landmarks[2].y);
    92 | int x = std::max(0, cx - crop_size / 2);
    93 | int y = std::max(0, cy - crop_size / 2);
    94 | int w = std::min(crop_size, image.cols - x);
    95 | int h = std::min(crop_size, image.rows - y);
    96 | cv::Mat crop = image(cv::Rect(x, y, w, h)).clone();
    97 | cv::resize(crop, crop, cv::Size(256, 256));
    98 | 99 | // Create masker and run inference
        100 | auto masker = create_occlusion_masker(
        101 | model_path, foundation::ai::inference_session::Options::with_best_providers());
    102 | ASSERT_NE(masker, nullptr);
    103 | 104 | cv::Mat mask = masker->create_occlusion_mask(crop);
    105 | 106 | // Verify mask properties
        107 | EXPECT_FALSE(mask.empty()) << "Occlusion mask should not be empty";
    108 | EXPECT_EQ(mask.type(), CV_8UC1) << "Mask should be single channel 8-bit";
    109 | EXPECT_EQ(mask.rows, 256) << "Mask height should match input";
    110 | EXPECT_EQ(mask.cols, 256) << "Mask width should match input";
    111 | 112 | // Save for visual inspection
        113 | cv::imwrite((output_dir / "occlusion_mask_result.png").string(), mask);
    114 |
}
115 | 116 | // ============================================================================
    117 |   // Region Masker Integration Tests
    118 |   // ============================================================================
    119 | 120 | TEST_F(FaceMaskerTest, CreateRegionMaskValidInputReturnsValidMask) {
    121 | // Get model path
        122 | std::string model_path = repo->ensure_model("bisenet_resnet_18");
    123 | if (model_path.empty()) {
        GTEST_SKIP() << "face_parser model not available";
    }
    124 | 125 | // Load test image
        126 | if (!fs::exists(test_image_path)) {
        127 | GTEST_SKIP() << "Test image not found: " << test_image_path;
        128 |
    }
    129 | cv::Mat image = cv::imread(test_image_path.string());
    130 | ASSERT_FALSE(image.empty()) << "Failed to load test image";
    131 | 132 | // Detect face and get landmarks
        133 | auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    134 | if (landmarks.empty()) {
        GTEST_SKIP() << "No face detected in test image";
    }
    135 | 136 | // Create a face crop
        137 | int crop_size = 512;
    138 | int cx = static_cast<int>(landmarks[2].x);
    139 | int cy = static_cast<int>(landmarks[2].y);
    140 | int x = std::max(0, cx - crop_size / 2);
    141 | int y = std::max(0, cy - crop_size / 2);
    142 | int w = std::min(crop_size, image.cols - x);
    143 | int h = std::min(crop_size, image.rows - y);
    144 | cv::Mat crop = image(cv::Rect(x, y, w, h)).clone();
    145 | cv::resize(crop, crop, cv::Size(512, 512));
    146 | 147 | // Create masker and run inference
        148 | auto masker = create_region_masker(
        149 | model_path, foundation::ai::inference_session::Options::with_best_providers());
    150 | ASSERT_NE(masker, nullptr);
    151 | 152 | // Test with skin and mouth regions
        153 | std::unordered_set<FaceRegion> regions = {FaceRegion::Skin, FaceRegion::Mouth};
    154 | cv::Mat mask = masker->create_region_mask(crop, regions);
    155 | 156 | // Verify mask properties
        157 | EXPECT_FALSE(mask.empty()) << "Region mask should not be empty";
    158 | EXPECT_EQ(mask.type(), CV_8UC1) << "Mask should be single channel 8-bit";
    159 | EXPECT_EQ(mask.rows, 512) << "Mask height should match input";
    160 | EXPECT_EQ(mask.cols, 512) << "Mask width should match input";
    161 | 162 | // Verify mask has some non-zero values (face regions detected)
        163 | int non_zero = cv::countNonZero(mask);
    164 | EXPECT_GT(non_zero, 0) << "Mask should have some selected regions";
    165 | 166 | // Save for visual inspection
        167 | cv::imwrite((output_dir / "region_mask_result.png").string(), mask);
    168 |
}
169 | 170 | TEST_F(FaceMaskerTest, CreateRegionMaskMultipleRegionsReturnsCombinedMask) {
    171 | std::string model_path = repo->ensure_model("bisenet_resnet_18");
    172 | if (model_path.empty()) {
        GTEST_SKIP() << "face_parser model not available";
    }
    173 | 174 | if (!fs::exists(test_image_path)) {
        GTEST_SKIP() << "Test image not found";
    }
    175 | 176 | cv::Mat image = cv::imread(test_image_path.string());
    177 | ASSERT_FALSE(image.empty());
    178 | 179 | auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    180 | if (landmarks.empty()) {
        GTEST_SKIP() << "No face detected";
    }
    181 | 182 | // Create crop
        183 | cv::resize(image, image, cv::Size(512, 512));
    184 | 185 | auto masker = create_region_masker(
        186 | model_path, foundation::ai::inference_session::Options::with_best_providers());
    187 | 188 | // Test different region combinations
        189
        | std::unordered_set<FaceRegion> eyes_only = {FaceRegion::LeftEye, FaceRegion::RightEye};
    190 | std::unordered_set<FaceRegion> full_face = {FaceRegion::Skin, FaceRegion::LeftEye,
                                                      191 | FaceRegion::RightEye, FaceRegion::Nose,
                                                      192 | FaceRegion::Mouth};
    193 | 194 | cv::Mat eyes_mask = masker->create_region_mask(image, eyes_only);
    195 | cv::Mat full_mask = masker->create_region_mask(image, full_face);
    196 | 197 | // Full face mask should have more non-zero pixels than eyes only
        198 | int eyes_count = cv::countNonZero(eyes_mask);
    199 | int full_count = cv::countNonZero(full_mask);
    200 | 201
        | EXPECT_GT(full_count, eyes_count)
              << "Full face mask should cover more area than eyes only";
    202 |
}
203 |