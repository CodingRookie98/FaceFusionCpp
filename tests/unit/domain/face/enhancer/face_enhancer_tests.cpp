#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.enhancer;
import domain.face.test_support;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::enhancer;
using namespace domain::face::test_support;
using namespace foundation::infrastructure::test;
namespace fs = std::filesystem;

class FaceEnhancerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        repo = setup_model_repository(assets_path);
        target_path = get_test_data_path("standard_face_test_images/lenna.bmp");
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    fs::path target_path;
};

TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceWithCodeFormer) {
    if (!fs::exists(target_path)) { GTEST_SKIP() << "Test image not found: " << target_path; }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = detect_face_landmarks(target_img, repo);
    if (target_kps.empty()) { GTEST_SKIP() << "No face detected in target image"; }

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::CodeFormer);

    std::string model_path = repo->ensure_model("codeformer");
    if (model_path.empty()) { GTEST_SKIP() << "CodeFormer model not found"; }

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    EnhanceInput input;
    input.target_frame = target_img;
    input.target_faces_landmarks = {target_kps};
    input.face_blend = 100; // Full enhancement

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(input);
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    EXPECT_EQ(result_img.size(), target_img.size());
    EXPECT_EQ(result_img.type(), target_img.type());

    // Save result for visual inspection
    fs::create_directories("tests_output");
    cv::imwrite("tests_output/enhance_codeformer_result.jpg", result_img);
}

TEST_F(FaceEnhancerIntegrationTest, EnhanceFaceWithGfpGan) {
    if (!fs::exists(target_path)) { GTEST_SKIP() << "Test image not found: " << target_path; }

    cv::Mat target_img = cv::imread(target_path.string());
    ASSERT_FALSE(target_img.empty());

    // 1. Prepare Input
    auto target_kps = detect_face_landmarks(target_img, repo);
    if (target_kps.empty()) { GTEST_SKIP() << "No face detected in target image"; }

    // 2. Create Enhancer
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::GfpGan);

    // Using gfpgan_1.4 as default test model
    std::string model_path = repo->ensure_model("gfpgan_1.4");
    if (model_path.empty()) { GTEST_SKIP() << "GFPGAN model not found"; }

    enhancer->load_model(model_path,
                         foundation::ai::inference_session::Options::with_best_providers());

    EnhanceInput input;
    input.target_frame = target_img;
    input.target_faces_landmarks = {target_kps};
    input.face_blend = 100;

    // 3. Run Enhancement
    cv::Mat result_img = enhancer->enhance_face(input);
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    EXPECT_EQ(result_img.size(), target_img.size());

    // Save result for visual inspection
    fs::create_directories("tests_output");
    cv::imwrite("tests_output/enhance_gfpgan_result.jpg", result_img);
}
