/**
 * @file face_expression_tests.cpp
 * @brief Unit tests for FaceExpressionRestorer
 * (LivePortrait).
 * @author CodingRookie
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.expression;
import domain.face.test_support;
import domain.face.helper;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::expression;
using namespace domain::face::test_support;
using namespace domain::face::helper;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class LivePortraitTest : public ::testing::Test {
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

TEST_F(LivePortraitTest, Construction) {
    EXPECT_NO_THROW({
        auto restorer = create_live_portrait_restorer();
        EXPECT_NE(restorer, nullptr);
    });
}

TEST_F(LivePortraitTest, RestoreExpressionBasic) {
    if (!fs::exists(source_path) || !fs::exists(target_path)) {
        GTEST_SKIP() << "Test images not found";
    }

    cv::Mat source_img = cv::imread(source_path.string());
    cv::Mat target_img = cv::imread(target_path.string());

    ASSERT_FALSE(source_img.empty());
    ASSERT_FALSE(target_img.empty());

    // 1. Detect Landmarks
    auto source_kps = detect_face_landmarks(source_img, repo);
    auto target_kps = detect_face_landmarks(target_img, repo);

    if (source_kps.empty() || target_kps.empty()) {
        GTEST_SKIP() << "Face detection failed for test images";
    }

    // 2. Create Restorer
    auto restorer = create_live_portrait_restorer();

    // 3. Load Models
    std::string feature_path = repo->ensure_model("live_portrait_feature_extractor");
    std::string motion_path = repo->ensure_model("live_portrait_motion_extractor");
    std::string generator_path = repo->ensure_model("live_portrait_generator");

    if (feature_path.empty() || motion_path.empty() || generator_path.empty()) {
        GTEST_SKIP() << "LivePortrait models not found";
    }

    EXPECT_NO_THROW(
        restorer->load_model(feature_path, motion_path, generator_path,
                             foundation::ai::inference_session::Options::with_best_providers()));

    // 4. Manual Crop
    auto [source_crop, _] = warp_face_by_face_landmarks_5(
        source_img, source_kps, WarpTemplateType::Arcface128V2, cv::Size(512, 512));

    auto [target_crop, __] = warp_face_by_face_landmarks_5(
        target_img, target_kps, WarpTemplateType::Arcface128V2, cv::Size(512, 512));

    // 5. Run Restoration
    cv::Mat result;
    EXPECT_NO_THROW(result = restorer->restore_expression(source_crop, target_crop, 0.5f));

    ASSERT_FALSE(result.empty());
    // Result size is crop size (512x512), not target frame size
    EXPECT_EQ(result.size(), cv::Size(512, 512));

    // Save output
    fs::create_directories("tests_output");
    cv::imwrite("tests_output/live_portrait_result.jpg", result);
}
