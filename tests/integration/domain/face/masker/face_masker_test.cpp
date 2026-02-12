/**
 * @file face_masker_tests.cpp
 * @brief Integration tests for FaceMasker.
 * @author CodingRookie
 *
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_set>
#include <filesystem>
#include <iostream>

import domain.face.masker;
import tests.helpers.domain.face_test_helpers;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import tests.helpers.foundation.test_utilities;
#include "common/test_paths.h"

using namespace domain::face::masker;
using namespace tests::helpers::domain;
using namespace tests::helpers::foundation;
namespace fs = std::filesystem;

extern void LinkGlobalTestEnvironment();

class FaceMaskerTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }

    void SetUp() override {
        auto assets_path = get_assets_path();
        repo = tests::helpers::domain::setup_model_repository(assets_path);
        test_image_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        output_dir = tests::common::TestPaths::GetTestOutputDir("face_masker");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    fs::path test_image_path;
    fs::path output_dir;
};

// ============================================================================
// Factory Exception Tests
// ============================================================================

TEST_F(FaceMaskerTest, CreateOcclusionMasker_EmptyPath_ThrowsException) {
    EXPECT_ANY_THROW(create_occlusion_masker(""));
}

TEST_F(FaceMaskerTest, CreateRegionMasker_EmptyPath_ThrowsException) {
    EXPECT_ANY_THROW(create_region_masker(""));
}

TEST_F(FaceMaskerTest, CreateOcclusionMasker_InvalidPath_ThrowsException) {
    EXPECT_ANY_THROW(create_occlusion_masker("invalid_path.onnx"));
}

TEST_F(FaceMaskerTest, CreateRegionMasker_InvalidPath_ThrowsException) {
    EXPECT_ANY_THROW(create_region_masker("invalid_path.onnx"));
}

// ============================================================================
// Occlusion Masker Integration Tests
// ============================================================================

TEST_F(FaceMaskerTest, CreateOcclusionMask_ValidInput_ReturnsValidMask) {
    // Get model path
    std::string model_path = repo->ensure_model("xseg_1");
    if (model_path.empty()) { GTEST_SKIP() << "face_occluder model not available"; }

    // Load test image
    if (!fs::exists(test_image_path)) {
        GTEST_SKIP() << "Test image not found: " << test_image_path;
    }
    cv::Mat image = cv::imread(test_image_path.string());
    ASSERT_FALSE(image.empty()) << "Failed to load test image";

    // Detect face and get landmarks
    auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    if (landmarks.empty()) { GTEST_SKIP() << "No face detected in test image"; }

    // Create a face crop (simplified - using center crop for testing)
    // In real usage, the face would be aligned using landmarks
    int crop_size = 256;
    int cx = static_cast<int>(landmarks[2].x); // nose point
    int cy = static_cast<int>(landmarks[2].y);
    int x = std::max(0, cx - crop_size / 2);
    int y = std::max(0, cy - crop_size / 2);
    int w = std::min(crop_size, image.cols - x);
    int h = std::min(crop_size, image.rows - y);
    cv::Mat crop = image(cv::Rect(x, y, w, h)).clone();
    cv::resize(crop, crop, cv::Size(256, 256));

    // Create masker and run inference
    auto masker = create_occlusion_masker(
        model_path, foundation::ai::inference_session::Options::with_best_providers());
    ASSERT_NE(masker, nullptr);

    cv::Mat mask = masker->create_occlusion_mask(crop);

    // Verify mask properties
    EXPECT_FALSE(mask.empty()) << "Occlusion mask should not be empty";
    EXPECT_EQ(mask.type(), CV_8UC1) << "Mask should be single channel 8-bit";
    EXPECT_EQ(mask.rows, 256) << "Mask height should match input";
    EXPECT_EQ(mask.cols, 256) << "Mask width should match input";

    // Save for visual inspection
    cv::imwrite((output_dir / "occlusion_mask_result.png").string(), mask);
}

// ============================================================================
// Region Masker Integration Tests
// ============================================================================

TEST_F(FaceMaskerTest, CreateRegionMask_ValidInput_ReturnsValidMask) {
    // Get model path
    std::string model_path = repo->ensure_model("bisenet_resnet_18");
    if (model_path.empty()) { GTEST_SKIP() << "face_parser model not available"; }

    // Load test image
    if (!fs::exists(test_image_path)) {
        GTEST_SKIP() << "Test image not found: " << test_image_path;
    }
    cv::Mat image = cv::imread(test_image_path.string());
    ASSERT_FALSE(image.empty()) << "Failed to load test image";

    // Detect face and get landmarks
    auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    if (landmarks.empty()) { GTEST_SKIP() << "No face detected in test image"; }

    // Create a face crop
    int crop_size = 512;
    int cx = static_cast<int>(landmarks[2].x);
    int cy = static_cast<int>(landmarks[2].y);
    int x = std::max(0, cx - crop_size / 2);
    int y = std::max(0, cy - crop_size / 2);
    int w = std::min(crop_size, image.cols - x);
    int h = std::min(crop_size, image.rows - y);
    cv::Mat crop = image(cv::Rect(x, y, w, h)).clone();
    cv::resize(crop, crop, cv::Size(512, 512));

    // Create masker and run inference
    auto masker = create_region_masker(
        model_path, foundation::ai::inference_session::Options::with_best_providers());
    ASSERT_NE(masker, nullptr);

    // Test with skin and mouth regions
    std::unordered_set<FaceRegion> regions = {FaceRegion::Skin, FaceRegion::Mouth};
    cv::Mat mask = masker->create_region_mask(crop, regions);

    // Verify mask properties
    EXPECT_FALSE(mask.empty()) << "Region mask should not be empty";
    EXPECT_EQ(mask.type(), CV_8UC1) << "Mask should be single channel 8-bit";
    EXPECT_EQ(mask.rows, 512) << "Mask height should match input";
    EXPECT_EQ(mask.cols, 512) << "Mask width should match input";

    // Verify mask has some non-zero values (face regions detected)
    int non_zero = cv::countNonZero(mask);
    EXPECT_GT(non_zero, 0) << "Mask should have some selected regions";

    // Save for visual inspection
    cv::imwrite((output_dir / "region_mask_result.png").string(), mask);
}

TEST_F(FaceMaskerTest, CreateRegionMask_MultipleRegions_ReturnsCombinedMask) {
    std::string model_path = repo->ensure_model("bisenet_resnet_18");
    if (model_path.empty()) { GTEST_SKIP() << "face_parser model not available"; }

    if (!fs::exists(test_image_path)) { GTEST_SKIP() << "Test image not found"; }

    cv::Mat image = cv::imread(test_image_path.string());
    ASSERT_FALSE(image.empty());

    auto landmarks = tests::helpers::domain::detect_face_landmarks(image, repo);
    if (landmarks.empty()) { GTEST_SKIP() << "No face detected"; }

    // Create crop
    cv::resize(image, image, cv::Size(512, 512));

    auto masker = create_region_masker(
        model_path, foundation::ai::inference_session::Options::with_best_providers());

    // Test different region combinations
    std::unordered_set<FaceRegion> eyes_only = {FaceRegion::LeftEye, FaceRegion::RightEye};
    std::unordered_set<FaceRegion> full_face = {FaceRegion::Skin, FaceRegion::LeftEye,
                                                FaceRegion::RightEye, FaceRegion::Nose,
                                                FaceRegion::Mouth};

    cv::Mat eyes_mask = masker->create_region_mask(image, eyes_only);
    cv::Mat full_mask = masker->create_region_mask(image, full_face);

    // Full face mask should have more non-zero pixels than eyes only
    int eyes_count = cv::countNonZero(eyes_mask);
    int full_count = cv::countNonZero(full_mask);

    EXPECT_GT(full_count, eyes_count) << "Full face mask should cover more area than eyes only";
}
