/**
 * @file face_enhancer_tests.cpp
 * @brief Integration tests for FaceEnhancer.
 * @author CodingRookie
 * * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.enhancer;
import tests.helpers.domain.face_test_helpers;
import domain.face.helper;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import tests.helpers.foundation.test_utilities;
#include "common/test_paths.h"

using namespace domain::face::enhancer;
using namespace tests::helpers::domain;
using namespace tests::helpers::foundation;
using namespace domain::face::helper;
namespace fs = std::filesystem;

extern void LinkGlobalTestEnvironment();

class FaceEnhancerIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        auto assets_path = get_assets_path();
        repo = tests::helpers::domain::setup_model_repository(assets_path);
        target_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        output_dir = tests::common::TestPaths::GetTestOutputDir("face_enhancer");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    fs::path target_path;
    fs::path output_dir;
};

TEST_F(FaceEnhancerIntegrationTest, EnhanceFace_CodeFormerModel_ProducesValidOutput) {
    if (!fs::exists(target_path)) { GTEST_SKIP() << "Test image not found: " << target_path; }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = tests::helpers::domain::detect_face_landmarks(target_img, repo);
    if (target_kps.empty()) { GTEST_SKIP() << "No face detected in target image"; }

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::CodeFormer);

    std::string model_path = repo->ensure_model("codeformer");
    if (model_path.empty()) { GTEST_SKIP() << "CodeFormer model not found"; }

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    // Manual crop for test
    auto [crop, _] = warp_face_by_face_landmarks_5(target_img, target_kps,
                                                   WarpTemplateType::Ffhq512, cv::Size(512, 512));

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(crop);

    // 4. Verify Result
    EXPECT_FALSE(result_img.empty());
    EXPECT_EQ(result_img.type(), target_img.type());

    // Save result for visual inspection
    cv::imwrite((output_dir / "enhance_codeformer_result.jpg").string(), result_img);
}

TEST_F(FaceEnhancerIntegrationTest, EnhanceFace_GfpGanModel_ProducesValidOutput) {
    if (!fs::exists(target_path)) { GTEST_SKIP() << "Test image not found: " << target_path; }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = tests::helpers::domain::detect_face_landmarks(target_img, repo);
    if (target_kps.empty()) { GTEST_SKIP() << "No face detected in target image"; }

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::GfpGan);

    // Using gfpgan_1.4 as default test model
    std::string model_path = repo->ensure_model("gfpgan_1.4");
    if (model_path.empty()) { GTEST_SKIP() << "GFPGAN model not found"; }

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    // Manual crop for test
    auto [crop, _] = warp_face_by_face_landmarks_5(target_img, target_kps,
                                                   WarpTemplateType::Ffhq512, cv::Size(512, 512));

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(crop);

    // 4. Verify Result
    EXPECT_FALSE(result_img.empty());

    // Save result for visual inspection
    cv::imwrite((output_dir / "enhance_gfpgan_result.jpg").string(), result_img);
}
