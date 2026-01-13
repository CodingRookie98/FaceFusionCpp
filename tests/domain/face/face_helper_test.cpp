#include <gtest/gtest.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <array>

import domain.face;
import domain.face.helper;

using namespace domain::face;
using namespace domain::face::helper;

TEST(FaceHelperTest, GetIoU) {
    // Overlapping boxes
    cv::Rect2f box1(0, 0, 10, 10);
    cv::Rect2f box2(5, 0, 10, 10);
    // Intersection: 5x10 = 50
    // Union: 100 + 100 - 50 = 150
    // IoU: 50/150 = 0.333...
    float iou = get_iou(box1, box2);
    EXPECT_NEAR(iou, 1.0f / 3.0f, 1e-5);

    // Non-overlapping boxes
    cv::Rect2f box3(0, 0, 10, 10);
    cv::Rect2f box4(20, 20, 10, 10);
    EXPECT_FLOAT_EQ(get_iou(box3, box4), 0.0f);

    // Identical boxes
    EXPECT_FLOAT_EQ(get_iou(box1, box1), 1.0f);
}

TEST(FaceHelperTest, ApplyNMS) {
    std::vector<cv::Rect2f> boxes = {
        {0, 0, 10, 10},   // Box A
        {1, 1, 10, 10},   // Box B (High overlap with A)
        {20, 20, 10, 10}, // Box C (No overlap)
        {21, 21, 10, 10}  // Box D (High overlap with C)
    };
    std::vector<float> scores = {0.9f, 0.8f, 0.7f, 0.6f};

    // A(0.9) suppresses B(0.8)
    // C(0.7) suppresses D(0.6)
    // Result should be indices of A and C: 0 and 2
    auto kept = apply_nms(boxes, scores, 0.5f);

    ASSERT_EQ(kept.size(), 2);
    EXPECT_EQ(kept[0], 0);
    EXPECT_EQ(kept[1], 2);
}

TEST(FaceHelperTest, ConvertLandmark68To5) {
    types::Landmarks kps68(68);
    // Fill with dummy data
    for (int i = 0; i < 68; ++i) {
        kps68[i] = cv::Point2f(static_cast<float>(i), static_cast<float>(i));
    }

    // Expected mapping logic from implementation:
    // Left Eye: Avg of 36-41
    // Right Eye: Avg of 42-47
    // Nose: 30
    // Left Mouth: 48
    // Right Mouth: 54

    // 36+37+38+39+40+41 = 231. 231/6 = 38.5
    cv::Point2f expectedLeftEye(38.5f, 38.5f);
    // 42+43+44+45+46+47 = 267. 267/6 = 44.5
    cv::Point2f expectedRightEye(44.5f, 44.5f);

    cv::Point2f expectedNose = kps68[30];
    cv::Point2f expectedLeftMouth = kps68[48];
    cv::Point2f expectedRightMouth = kps68[54];

    auto kps5 = convert_face_landmark_68_to_5(kps68);

    ASSERT_EQ(kps5.size(), 5);
    EXPECT_FLOAT_EQ(kps5[0].x, expectedLeftEye.x);
    EXPECT_FLOAT_EQ(kps5[0].y, expectedLeftEye.y);
    EXPECT_FLOAT_EQ(kps5[1].x, expectedRightEye.x);
    EXPECT_FLOAT_EQ(kps5[1].y, expectedRightEye.y);
    EXPECT_EQ(kps5[2], expectedNose);
    EXPECT_EQ(kps5[3], expectedLeftMouth);
    EXPECT_EQ(kps5[4], expectedRightMouth);
}

TEST(FaceHelperTest, CreateStaticAnchors) {
    // featureStride=8, anchorTotal=2, strideHeight=2, strideWidth=2
    // Grid: (0,0), (0,8), (8,0), (8,8)
    // Each grid point generates 2 anchors
    // Total anchors = 2 * 2 * 2 = 8
    auto anchors = create_static_anchors(8, 2, 2, 2);

    ASSERT_EQ(anchors.size(), 8);
    // Check first couple
    EXPECT_EQ(anchors[0][0], 0);
    EXPECT_EQ(anchors[0][1], 0); // (0,0) anchor 1
    EXPECT_EQ(anchors[1][0], 0);
    EXPECT_EQ(anchors[1][1], 0); // (0,0) anchor 2
    EXPECT_EQ(anchors[2][0], 0);
    EXPECT_EQ(anchors[2][1], 8); // (0,8) anchor 1
}

TEST(FaceHelperTest, CalcAverageEmbedding) {
    std::vector<std::vector<float>> embeddings = {{1.0f, 2.0f, 3.0f}, {3.0f, 2.0f, 1.0f}};

    auto avg = calc_average_embedding(embeddings);
    ASSERT_EQ(avg.size(), 3);
    EXPECT_FLOAT_EQ(avg[0], 2.0f);
    EXPECT_FLOAT_EQ(avg[1], 2.0f);
    EXPECT_FLOAT_EQ(avg[2], 2.0f);
}
