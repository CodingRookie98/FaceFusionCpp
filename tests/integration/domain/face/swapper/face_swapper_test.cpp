1 | /**
  2| * @file face_swapper_tests.cpp
  3| * @brief Integration tests for FaceSwapper.
  4| * @author CodingRookie
  5| *
  6| * @date 2026-01-27
  7| */
    8 | 9 | #include < gtest / gtest.h > 10 | #include < opencv2 / core.hpp > 11
    | #include < opencv2 / imgcodecs.hpp > 12 | #include < filesystem > 13 | #include < vector > 14
    | #include < iostream > 15 | 16 | import domain.face.swapper;
17 | import domain.face.masker;
18 | import tests.helpers.domain.face_test_helpers;
19 | import domain.face.helper;
20 | import domain.ai.model_repository;
21 | import foundation.ai.inference_session;
22 | import tests.helpers.foundation.test_utilities;
23 | #include "common/test_paths.h" 24 | 25 | using namespace domain::face::swapper;
26 | using namespace domain::face::masker;
27 | using namespace tests::helpers::domain;
28 | using namespace domain::face::helper;
29 | using namespace tests::helpers::foundation;
30 | namespace fs = std::filesystem;
31 | 32 | extern void LinkGlobalTestEnvironment();
33 | 34 | class FaceSwapperIntegrationTest : public ::testing::Test {
    35 | protected : 36 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    37 | 38 | void SetUp() override {
        39 | auto assets_path = get_assets_path();
        40 | repo = tests::helpers::domain::setup_model_repository(assets_path);
        41 | source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        42 | target_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
        43 | output_dir = tests::common::TestPaths::GetTestOutputDir("face_swapper");
        44 |
    }
    45 | 46 | std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    47 | fs::path source_path;
    48 | fs::path target_path;
    49 | fs::path output_dir;
    50 |
};
51 | 52 | TEST_F(FaceSwapperIntegrationTest, SwapFaceValidInputResultResemblesSource) {
    53 | if (!fs::exists(source_path) || !fs::exists(target_path)) {
        54 | GTEST_SKIP() << "Test images not found";
        55 |
    }
    56 | 57 | cv::Mat source_img = cv::imread(source_path.string());
    58 | cv::Mat target_img = cv::imread(target_path.string());
    59 | 60 | ASSERT_FALSE(source_img.empty());
    61 | ASSERT_FALSE(target_img.empty());
    62 | 63 | // 1. Extract Source Embedding
        64 | auto source_kps = tests::helpers::domain::detect_face_landmarks(source_img, repo);
    65 | if (source_kps.empty()) {
        GTEST_SKIP() << "No face detected in source image";
    }
    66 | 67 | auto source_embedding = get_face_embedding(source_img, source_kps, repo);
    68 | ASSERT_FALSE(source_embedding.empty()) << "Failed to extract source embedding";
    69 | 70 | // 2. Prepare Target
        71 | auto target_kps = tests::helpers::domain::detect_face_landmarks(target_img, repo);
    72 | if (target_kps.empty()) {
        GTEST_SKIP() << "No face detected in target image";
    }
    73 | 74 | // 3. Run Swapper
        75 | auto swapper = FaceSwapperFactory::create_inswapper();
    76 | // Correct key from models_info.json is "inswapper_128"
        77 | std::string swapper_model_path = repo->ensure_model("inswapper_128");
    78 | if (swapper_model_path.empty()) {
        GTEST_SKIP() << "Swapper model not found";
    }
    79 | 80
        | swapper->load_model(
            swapper_model_path,
            81 | foundation::ai::inference_session::Options::with_best_providers());
    82 | 83 | // Manual Crop
        84 | auto [target_crop, _] = warp_face_by_face_landmarks_5(
        85 | target_img, target_kps, WarpTemplateType::Arcface128V2, cv::Size(128, 128));
    86 | 87 | // Act
        88 | cv::Mat result_img = swapper->swap_face(target_crop, source_embedding);
    89 | 90 | // Assert
        91 | ASSERT_FALSE(result_img.empty());
    92 | 93 | // 4. Verify Result
        94 |  // Extract embedding from result face
        95 | auto result_kps = tests::helpers::domain::detect_face_landmarks(result_img, repo);
    96 | ASSERT_FALSE(result_kps.empty()) << "No face detected in result image";
    97 | auto result_embedding = get_face_embedding(result_img, result_kps, repo);
    98 | 99 | // Calculate Cosine Similarity with Source Embedding
        100 | double similarity = 0.0;
    101 | for (size_t i = 0; i < source_embedding.size(); ++i) {
        102 | similarity += source_embedding[i] * result_embedding[i];
        103 |
    }
    104 | 105 | std::cout << "Swap Similarity: " << similarity << std::endl;
    106 | 107 | // Expect reasonable similarity (usually > 0.3 or 0.4 for swap result vs source)
        108 | EXPECT_GT(similarity, 0.3) << "Swapped face should resemble source face";
    109 | 110 | // Save result for visual inspection
        111 | cv::imwrite((output_dir / "swap_test_result.jpg").string(), result_img);
    112 |
}
113 |