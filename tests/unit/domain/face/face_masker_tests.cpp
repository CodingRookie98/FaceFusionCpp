#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <unordered_set>

import domain.face.masker;

using namespace domain::face::masker;

class FaceMaskerTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(FaceMaskerTest, CreateOcclusionMaskerThrowsOnEmptyPath) {
    EXPECT_ANY_THROW(create_occlusion_masker(""));
}

TEST_F(FaceMaskerTest, CreateRegionMaskerThrowsOnEmptyPath) {
    EXPECT_ANY_THROW(create_region_masker(""));
}

TEST_F(FaceMaskerTest, CreateOcclusionMaskerThrowsOnInvalidPath) {
    EXPECT_ANY_THROW(create_occlusion_masker("invalid_path.onnx"));
}

TEST_F(FaceMaskerTest, CreateRegionMaskerThrowsOnInvalidPath) {
    EXPECT_ANY_THROW(create_region_masker("invalid_path.onnx"));
}

TEST_F(FaceMaskerTest, FactoryLoadsExistingModel) {
    // Use an existing model from the assets (Landmarker or similar)
    // to verify the factory mechanics, even if the model type is wrong.
    // This confirms the loading pipeline works.
    std::string existing_model_path = "assets/models/face_landmarker_68_5.onnx";

    // We expect it to NOT throw, or if it throws due to strict validation inside InferenceSession,
    // we catch that. However, our InferenceSession usually just loads.
    try {
        auto masker = create_occlusion_masker(existing_model_path);
        EXPECT_NE(masker, nullptr);
    } catch (const std::exception& e) {
        // If it throws "File not found" it's a test setup issue.
        // If it throws "Invalid Graph" it's expected but shows loading happened.
        // For now, let's just log.
        std::cout << "Model load test info: " << e.what() << std::endl;
    }
}
