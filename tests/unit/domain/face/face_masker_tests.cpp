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
