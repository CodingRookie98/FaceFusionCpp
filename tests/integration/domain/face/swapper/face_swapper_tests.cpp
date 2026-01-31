/**
 * @file face_swapper_tests.cpp
 * @brief Unit tests for FaceSwapper.
 * @author CodingRookie
 *
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.swapper;
import domain.face.masker;
import domain.face.test_support;
import domain.face.helper;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::swapper;
using namespace domain::face::masker;
using namespace domain::face::test_support;
using namespace domain::face::helper;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class FaceSwapperIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        repo = setup_model_repository(assets_path);
        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        target_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    fs::path source_path;
    fs::path target_path;
};

TEST_F(FaceSwapperIntegrationTest, SwapFaceAndVerifySimilarity) {
    if (!fs::exists(source_path) || !fs::exists(target_path)) {
        GTEST_SKIP() << "Test images not found";
    }

    cv::Mat source_img = cv::imread(source_path.string());
    cv::Mat target_img = cv::imread(target_path.string());

    ASSERT_FALSE(source_img.empty());
    ASSERT_FALSE(target_img.empty());

    // 1. Extract Source Embedding
    auto source_kps = detect_face_landmarks(source_img, repo);
    if (source_kps.empty()) { GTEST_SKIP() << "No face detected in source image"; }

    auto source_embedding = get_face_embedding(source_img, source_kps, repo);
    ASSERT_FALSE(source_embedding.empty()) << "Failed to extract source embedding";

    // 2. Prepare Target
    auto target_kps = detect_face_landmarks(target_img, repo);
    if (target_kps.empty()) { GTEST_SKIP() << "No face detected in target image"; }

    // 3. Run Swapper
    auto swapper = FaceSwapperFactory::create_inswapper();
    // Correct key from models_info.json is "inswapper_128"
    std::string swapper_model_path = repo->ensure_model("inswapper_128");
    if (swapper_model_path.empty()) { GTEST_SKIP() << "Swapper model not found"; }

    swapper->load_model(swapper_model_path,
                        foundation::ai::inference_session::Options::with_best_providers());

    // Manual Crop
    auto [target_crop, _] = warp_face_by_face_landmarks_5(
        target_img, target_kps, WarpTemplateType::Arcface_128_v2, cv::Size(128, 128));

    // Act
    cv::Mat result_img = swapper->swap_face(target_crop, source_embedding);

    // Assert
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    // Extract embedding from result face
    auto result_kps = detect_face_landmarks(result_img, repo);
    ASSERT_FALSE(result_kps.empty()) << "No face detected in result image";
    auto result_embedding = get_face_embedding(result_img, result_kps, repo);

    // Calculate Cosine Similarity with Source Embedding
    double similarity = 0.0;
    for (size_t i = 0; i < source_embedding.size(); ++i) {
        similarity += source_embedding[i] * result_embedding[i];
    }

    std::cout << "Swap Similarity: " << similarity << std::endl;

    // Expect reasonable similarity (usually > 0.3 or 0.4 for swap result vs source)
    EXPECT_GT(similarity, 0.3) << "Swapped face should resemble source face";

    // Save result for visual inspection
    fs::create_directories("tests_output");
    cv::imwrite("tests_output/swap_test_result.jpg", result_img);
}
