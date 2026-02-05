#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_set>

import domain.face.masker;
import domain.face;

using namespace domain::face::masker;
using namespace domain::face::types;
using namespace testing;

class MockFaceOccluder : public IFaceOccluder {
public:
    MOCK_METHOD(cv::Mat, create_occlusion_mask, (const cv::Mat&), (override));
};

class MockFaceRegionMasker : public IFaceRegionMasker {
public:
    MOCK_METHOD(cv::Mat, create_region_mask, (const cv::Mat&, const std::unordered_set<FaceRegion>&), (override));
};

class MaskCompositorTest : public Test {
protected:
    void SetUp() override {
        input.size = cv::Size(100, 100);
        input.crop_frame = cv::Mat::zeros(100, 100, CV_8UC3);
        input.options.mask_types = {}; // Start with no masks
        input.occluder = &mock_occluder;
        input.region_masker = &mock_region_masker;
    }

    MaskCompositor::CompositionInput input;
    NiceMock<MockFaceOccluder> mock_occluder;
    NiceMock<MockFaceRegionMasker> mock_region_masker;
};

TEST_F(MaskCompositorTest, ComposeNoMaskReturnsOnes) {
    input.options.mask_types = {};
    cv::Mat result = MaskCompositor::compose(input);
    
    EXPECT_EQ(result.size(), input.size);
    EXPECT_EQ(result.type(), CV_32FC1);
    // Should be all white (1.0)
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);
    EXPECT_NEAR(minVal, 1.0, 1e-6);
    EXPECT_NEAR(maxVal, 1.0, 1e-6);
}

TEST_F(MaskCompositorTest, ComposeBoxMaskWithPadding) {
    input.options.mask_types = {MaskType::Box};
    input.options.box_mask_blur = 0.0f; // Disable blur for easier checking
    // Top 10%, Right 0%, Bottom 0%, Left 0%
    input.options.box_mask_padding = {10, 0, 0, 0};
    
    cv::Mat result = MaskCompositor::compose(input);
    
    // Check top region (should be 0) - avoid boundary due to final GaussianBlur
    // pad_top is 10. With 3x3 kernel, lines 0-8 should be safe.
    cv::Mat top_region = result(cv::Rect(0, 0, 100, 8));
    double minVal, maxVal;
    cv::minMaxLoc(top_region, &minVal, &maxVal);
    EXPECT_NEAR(maxVal, 0.0, 1e-6);
    
    // Check center region (should be 1) - avoid boundary
    // Note: pad_left, pad_right, pad_bottom default to 1 due to blur_area logic even if padding is 0
    // So we need to crop left/right/bottom as well.
    cv::Mat center_region = result(cv::Rect(5, 15, 90, 80));
    cv::minMaxLoc(center_region, &minVal, &maxVal);
    EXPECT_NEAR(minVal, 1.0, 1e-6);
}

TEST_F(MaskCompositorTest, ComposeOcclusionMaskInvertsResult) {
    input.options.mask_types = {MaskType::Occlusion};
    
    // Mock occlusion mask: center 50x50 is occluded (255)
    cv::Mat occlusion_mask = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(occlusion_mask, cv::Rect(25, 25, 50, 50), cv::Scalar(255), -1);
    
    EXPECT_CALL(mock_occluder, create_occlusion_mask(_))
        .WillOnce(Return(occlusion_mask));
        
    cv::Mat result = MaskCompositor::compose(input);
    
    // The compositor inverts the occlusion mask:
    // Input 255 (occluded) -> Output 0 (keep original/don't swap)
    // Input 0 (clear) -> Output 255/1.0 (swap)
    
    // Center should be 0.0 (masked out)
    float center_val = result.at<float>(50, 50);
    EXPECT_NEAR(center_val, 0.0f, 1e-6);
    
    // Corner should be 1.0 (swapped)
    float corner_val = result.at<float>(0, 0);
    EXPECT_NEAR(corner_val, 1.0f, 1e-6);
}

TEST_F(MaskCompositorTest, ComposeRegionMaskUsesDirectResult) {
    input.options.mask_types = {MaskType::Region};
    input.options.regions = {FaceRegion::Skin};
    
    // Mock region mask: center 50x50 is selected (255)
    cv::Mat region_mask = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(region_mask, cv::Rect(25, 25, 50, 50), cv::Scalar(255), -1);
    
    EXPECT_CALL(mock_region_masker, create_region_mask(_, _))
        .WillOnce(Return(region_mask));
        
    cv::Mat result = MaskCompositor::compose(input);
    
    // Center should be 1.0 (selected)
    float center_val = result.at<float>(50, 50);
    EXPECT_NEAR(center_val, 1.0f, 1e-6);
    
    // Corner should be 0.0 (not selected)
    float corner_val = result.at<float>(0, 0);
    EXPECT_NEAR(corner_val, 0.0f, 1e-6);
}

TEST_F(MaskCompositorTest, ComposeCombinedMasksUsesIntersection) {
    input.options.mask_types = {MaskType::Box, MaskType::Region};
    input.options.box_mask_blur = 0.0f;
    input.options.box_mask_padding = {50, 0, 0, 0}; // Top half masked out (0)
    
    // Region mask: Right half selected (255)
    cv::Mat region_mask = cv::Mat::zeros(100, 100, CV_8UC1);
    cv::rectangle(region_mask, cv::Rect(50, 0, 50, 100), cv::Scalar(255), -1);
    
    EXPECT_CALL(mock_region_masker, create_region_mask(_, _))
        .WillOnce(Return(region_mask));
        
    cv::Mat result = MaskCompositor::compose(input);
    
    // Intersection:
    // Top Left: Box=0, Region=0 -> 0
    // Top Right: Box=0, Region=1 -> 0
    // Bottom Left: Box=1, Region=0 -> 0
    // Bottom Right: Box=1, Region=1 -> 1
    
    EXPECT_NEAR(result.at<float>(25, 25), 0.0f, 1e-6); // Top Left
    EXPECT_NEAR(result.at<float>(25, 75), 0.0f, 1e-6); // Top Right
    EXPECT_NEAR(result.at<float>(75, 25), 0.0f, 1e-6); // Bottom Left
    EXPECT_NEAR(result.at<float>(75, 75), 1.0f, 1e-6); // Bottom Right
}

TEST_F(MaskCompositorTest, ComposeEmptyMaskListReturnsOnes) {
    input.options.mask_types = {};
    cv::Mat result = MaskCompositor::compose(input);
    double minVal, maxVal;
    cv::minMaxLoc(result, &minVal, &maxVal);
    EXPECT_NEAR(minVal, 1.0, 1e-6);
}
