1 | /**
  2| * @file face_enhancer_tests.cpp
  3| * @brief Integration tests for FaceEnhancer.
  4| * @author CodingRookie
  5| * * @date 2026-01-27
  6| */
    7 | 8 | #include < gtest / gtest.h > 9 | #include < opencv2 / core.hpp > 10
    | #include < opencv2 / imgcodecs.hpp > 11 | #include < filesystem > 12 | #include < vector > 13
    | #include < iostream > 14 | 15 | import domain.face.enhancer;
16 | import tests.helpers.domain.face_test_helpers;
17 | import domain.face.helper;
18 | import domain.ai.model_repository;
19 | import foundation.ai.inference_session;
20 | import tests.helpers.foundation.test_utilities;
21 | #include "common/test_paths.h" 22 | 23 | using namespace domain::face::enhancer;
24 | using namespace tests::helpers::domain;
25 | using namespace tests::helpers::foundation;
26 | using namespace domain::face::helper;
27 | namespace fs = std::filesystem;
28 | 29 | extern void LinkGlobalTestEnvironment();
30 | 31 | class FaceEnhancerIntegrationTest : public ::testing::Test {
    32 | protected : 33 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    34 | 35 | void SetUp() override {
        36 | auto assets_path = get_assets_path();
        37 | repo = tests::helpers::domain::setup_model_repository(assets_path);
        38 | target_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        39 | output_dir = tests::common::TestPaths::GetTestOutputDir("face_enhancer");
        40 |
    }
    41 | 42 | std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    43 | fs::path target_path;
    44 | fs::path output_dir;
    45 |
};
46 | 47 | TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceCodeFormerModelProducesValidOutput) {
    48 | if (!fs::exists(target_path)) {
        GTEST_SKIP() << "Test image not found: " << target_path;
    }
    49 | 50 | cv::Mat target_img = cv::imread(target_path.string());
    51 | ASSERT_FALSE(target_img.empty());
    52 | 53 | // 1. Prepare Input
        54 | auto target_kps = tests::helpers::domain::detect_face_landmarks(target_img, repo);
    55 | if (target_kps.empty()) {
        GTEST_SKIP() << "No face detected in target image";
    }
    56 | 57 | // 2. Create Enhancer
        58 | auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::CodeFormer);
    59 | 60 | std::string model_path = repo->ensure_model("codeformer");
    61 | if (model_path.empty()) {
        GTEST_SKIP() << "CodeFormer model not found";
    }
    62 | 63
        | enhancer->load_model(
            model_path, 64 | foundation::ai::inference_session::Options::with_best_providers());
    65 | 66 | // Manual crop for test
        67 | auto [crop, _] = warp_face_by_face_landmarks_5(
        target_img, target_kps, 68 | WarpTemplateType::Ffhq512, cv::Size(512, 512));
    69 | 70 | // 3. Run Enhancement
        71 | cv::Mat result_img = enhancer->enhance_face(crop);
    72 | 73 | // 4. Verify Result
        74 | EXPECT_FALSE(result_img.empty());
    75 | EXPECT_EQ(result_img.type(), target_img.type());
    76 | 77 | // Save result for visual inspection
        78 | cv::imwrite((output_dir / "enhance_codeformer_result.jpg").string(), result_img);
    79 |
}
80 | 81 | TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceGfpGanModelProducesValidOutput) {
    82 | if (!fs::exists(target_path)) {
        GTEST_SKIP() << "Test image not found: " << target_path;
    }
    83 | 84 | cv::Mat target_img = cv::imread(target_path.string());
    85 | ASSERT_FALSE(target_img.empty());
    86 | 87 | // 1. Prepare Input
        88 | auto target_kps = tests::helpers::domain::detect_face_landmarks(target_img, repo);
    89 | if (target_kps.empty()) {
        GTEST_SKIP() << "No face detected in target image";
    }
    90 | 91 | // 2. Create Enhancer
        92 | auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::GfpGan);
    93 | 94 | // Using gfpgan_1.4 as default test model
        95 | std::string model_path = repo->ensure_model("gfpgan_1.4");
    96 | if (model_path.empty()) {
        GTEST_SKIP() << "GFPGAN model not found";
    }
    97 | 98
        | enhancer->load_model(
            model_path, 99 | foundation::ai::inference_session::Options::with_best_providers());
    100 | 101 | // Manual crop for test
        102 | auto [crop, _] = warp_face_by_face_landmarks_5(
        target_img, target_kps, 103 | WarpTemplateType::Ffhq512, cv::Size(512, 512));
    104 | 105 | // 3. Run Enhancement
        106 | cv::Mat result_img = enhancer->enhance_face(crop);
    107 | 108 | // 4. Verify Result
        109 | EXPECT_FALSE(result_img.empty());
    110 | 111 | // Save result for visual inspection
        112 | cv::imwrite((output_dir / "enhance_gfpgan_result.jpg").string(), result_img);
    113 |
}
114 |