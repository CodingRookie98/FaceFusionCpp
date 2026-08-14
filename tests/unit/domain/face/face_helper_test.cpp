#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>
#include <cstdint>
#include <string>

import domain.face.helper;
import domain.face;

using namespace domain::face::helper;
using namespace domain::face;

namespace {

// Helper to check floating point equality for vectors
void ExpectVectorsNear(const std::vector<float>& v1, const std::vector<float>& v2,
                       float abs_error = 1e-5) {
    ASSERT_EQ(v1.size(), v2.size());
    for (size_t i = 0; i < v1.size(); ++i) {
        EXPECT_NEAR(v1[i], v2[i], abs_error) << "Vector mismatch at index " << i;
    }
}

// Build a raw little-endian FP16 byte string from uint16 bit patterns
std::string MakeRawFp16(std::initializer_list<std::uint16_t> values) {
    std::string bytes;
    bytes.reserve(values.size() * 2);
    for (const std::uint16_t value : values) {
        bytes.push_back(static_cast<char>(value & 0xFF));
        bytes.push_back(static_cast<char>((value >> 8) & 0xFF));
    }
    return bytes;
}

} // namespace

class FaceHelperTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }
};

// --- IoU Tests ---

TEST_F(FaceHelperTest, GetIoUNoOverlap) {
    cv::Rect2f box1(0, 0, 10, 10);
    cv::Rect2f box2(20, 20, 10, 10);
    EXPECT_FLOAT_EQ(get_iou(box1, box2), 0.0f);
}

TEST_F(FaceHelperTest, GetIoUPerfectMatch) {
    cv::Rect2f box1(0, 0, 10, 10);
    EXPECT_FLOAT_EQ(get_iou(box1, box1), 1.0f);
}

TEST_F(FaceHelperTest, GetIoUPartialOverlap) {
    // Box1 area = 100
    // Box2 area = 100
    // Overlap: 5x10 = 50
    // Union: 100 + 100 - 50 = 150
    // IoU: 50/150 = 1/3
    cv::Rect2f box1(0, 0, 10, 10);
    cv::Rect2f box2(5, 0, 10, 10);
    EXPECT_NEAR(get_iou(box1, box2), 1.0f / 3.0f, 1e-5);
}

TEST_F(FaceHelperTest, GetIoUInside) {
    // Box1 area = 100
    // Box2 area = 25 (inside box1)
    // Overlap = 25
    // Union = 100
    // IoU = 25/100 = 0.25
    cv::Rect2f box1(0, 0, 10, 10);
    cv::Rect2f box2(2, 2, 5, 5);
    EXPECT_NEAR(get_iou(box1, box2), 0.25f, 1e-5);
}

// --- NMS Tests ---

TEST_F(FaceHelperTest, ApplyNMSFiltersOverlapping) {
    std::vector<cv::Rect2f> boxes = {
        cv::Rect2f(0, 0, 10, 10), // Box A
        cv::Rect2f(1, 1, 10, 10)  // Box B (high overlap with A)
    };
    std::vector<float> confidences = {0.9f, 0.8f}; // A has higher confidence
    float thresh = 0.5f;

    auto indices = apply_nms(boxes, confidences, thresh);

    ASSERT_EQ(indices.size(), 1);
    EXPECT_EQ(indices[0], 0); // Should keep Box A (index 0)
}

TEST_F(FaceHelperTest, ApplyNMSKeepsDistinct) {
    std::vector<cv::Rect2f> boxes = {
        cv::Rect2f(0, 0, 10, 10),  // Box A
        cv::Rect2f(20, 20, 10, 10) // Box B (no overlap)
    };
    std::vector<float> confidences = {0.9f, 0.8f};
    float thresh = 0.5f;

    auto indices = apply_nms(boxes, confidences, thresh);

    ASSERT_EQ(indices.size(), 2);
    // Indices might be sorted by score, so we check existence
    bool has_0 = std::find(indices.begin(), indices.end(), 0) != indices.end();
    bool has_1 = std::find(indices.begin(), indices.end(), 1) != indices.end();
    EXPECT_TRUE(has_0);
    EXPECT_TRUE(has_1);
}

// --- Interpolation Tests ---

TEST_F(FaceHelperTest, InterpStandard) {
    std::vector<float> x = {0.5f, 1.5f};
    std::vector<float> xp = {0.0f, 1.0f, 2.0f};
    std::vector<float> fp = {0.0f, 10.0f, 20.0f};

    // 0.5 is between 0 and 1 -> interp(0, 10) at 0.5 = 5.0
    // 1.5 is between 1 and 2 -> interp(10, 20) at 0.5 = 15.0
    auto result = interp(x, xp, fp);

    ExpectVectorsNear(result, {5.0f, 15.0f});
}

TEST_F(FaceHelperTest, InterpClamping) {
    std::vector<float> x = {-1.0f, 3.0f};
    std::vector<float> xp = {0.0f, 2.0f};
    std::vector<float> fp = {10.0f, 20.0f};

    auto result = interp(x, xp, fp);

    ExpectVectorsNear(result, {10.0f, 20.0f});
}

// --- Embedding Average Tests ---

TEST_F(FaceHelperTest, CalcAverageEmbedding) {
    std::vector<std::vector<float>> embeddings = {{1.0f, 2.0f}, {3.0f, 4.0f}};

    auto avg = calc_average_embedding(embeddings);

    ExpectVectorsNear(avg, {2.0f, 3.0f});
}

TEST_F(FaceHelperTest, ComputeAverageEmbeddingNormalized) {
    Face f1;
    f1.set_embedding({1.0f, 0.0f});
    Face f2;
    f2.set_embedding({0.0f, 1.0f});

    // Average raw: {0.5, 0.5}
    // Norm: sqrt(0.25 + 0.25) = sqrt(0.5) approx 0.707
    // Normalized: {0.5/0.707, 0.5/0.707} approx {0.707, 0.707}

    auto avg = compute_average_embedding({f1, f2});

    float val = 0.5f / std::sqrt(0.5f);
    ExpectVectorsNear(avg, {val, val});

    // Check if result is unit vector
    float norm_sq = avg[0] * avg[0] + avg[1] * avg[1];
    EXPECT_NEAR(norm_sq, 1.0f, 1e-5);
}

// --- Anchor Tests ---

TEST_F(FaceHelperTest, CreateStaticAnchors) {
    // feature_stride=8, anchor_total=2, h=2, w=2
    // Grid: (0,0), (0,1), (1,0), (1,1)
    // Coords: (0,0), (0,8), (8,0), (8,8)
    // 2 anchors per point -> 8 total
    auto anchors = create_static_anchors(8, 2, 2, 2);

    ASSERT_EQ(anchors.size(), 8);
    // Check first anchor at (0,0)
    EXPECT_EQ(anchors[0][0], 0);
    EXPECT_EQ(anchors[0][1], 0);
    // Check last anchor at (8,8)
    EXPECT_EQ(anchors[7][0], 8);
    EXPECT_EQ(anchors[7][1], 8);
}

TEST_F(FaceHelperTest, DistanceToBBox) {
    std::array<int, 2> anchor = {100, 100}; // y, x
    cv::Rect2f bbox(10, 10, 20, 20);        // x, y, w, h

    // result.x = anchor_x - bbox.x = 100 - 10 = 90
    // result.y = anchor_y - bbox.y = 100 - 10 = 90
    // result.w = bbox.x + bbox.w = 10 + 20 = 30
    // result.h = bbox.y + bbox.h = 10 + 20 = 30

    auto result = distance_2_bbox(anchor, bbox);

    EXPECT_FLOAT_EQ(result.x, 90.0f);
    EXPECT_FLOAT_EQ(result.y, 90.0f);
    EXPECT_FLOAT_EQ(result.width, 30.0f);
    EXPECT_FLOAT_EQ(result.height, 30.0f);
}

TEST_F(FaceHelperTest, DistanceToFaceLandmark5) {
    std::array<int, 2> anchor = {50, 50}; // y, x
    types::Landmarks kps(5);
    kps[0] = cv::Point2f(10, 10);

    auto result = distance_2_face_landmark_5(anchor, kps);

    // Should add anchor coords
    EXPECT_FLOAT_EQ(result[0].x, 60.0f);
    EXPECT_FLOAT_EQ(result[0].y, 60.0f);
}

// --- Rotation Tests ---

TEST_F(FaceHelperTest, RotatePointBack90) {
    // 90 degrees CCW
    // Original size 100x50 (WxH)
    // Point (10, 10)
    // Formula: (W - y, x) -> (100 - 10, 10) = (90, 10)
    cv::Size size(100, 50);
    cv::Point2f pt(10, 10);

    auto res = rotate_point_back(pt, 90, size);

    EXPECT_FLOAT_EQ(res.x, 90.0f);
    EXPECT_FLOAT_EQ(res.y, 10.0f);
}

TEST_F(FaceHelperTest, RotatePointBack180) {
    // 180 degrees
    // Formula: (W - x, H - y) -> (100 - 10, 50 - 10) = (90, 40)
    cv::Size size(100, 50);
    cv::Point2f pt(10, 10);

    auto res = rotate_point_back(pt, 180, size);

    EXPECT_FLOAT_EQ(res.x, 90.0f);
    EXPECT_FLOAT_EQ(res.y, 40.0f);
}

TEST_F(FaceHelperTest, RotatePointBack270) {
    // 270 degrees (90 CW)
    // Formula: (y, H - x) -> (10, 50 - 10) = (10, 40)
    cv::Size size(100, 50);
    cv::Point2f pt(10, 10);

    auto res = rotate_point_back(pt, 270, size);

    EXPECT_FLOAT_EQ(res.x, 10.0f);
    EXPECT_FLOAT_EQ(res.y, 40.0f);
}

TEST_F(FaceHelperTest, RotateBoxBack) {
    // Simple 180 degree check
    cv::Size size(100, 100);
    cv::Rect2f box(10, 10, 20, 20);
    // Top-Left: (10, 10) -> (90, 90)
    // Bottom-Right: (30, 30) -> (70, 70)
    // New box should be from (70, 70) with size 20x20

    auto res = rotate_box_back(box, 180, size);

    EXPECT_FLOAT_EQ(res.x, 70.0f);
    EXPECT_FLOAT_EQ(res.y, 70.0f);
    EXPECT_FLOAT_EQ(res.width, 20.0f);
    EXPECT_FLOAT_EQ(res.height, 20.0f);
}

// --- FP16 to FP32 conversion tests ---

TEST_F(FaceHelperTest, ConvertFp16ToFp32BasicValues) {
    // 1.0 = 0x3C00, 0.5 = 0x3800, -2.0 = 0xC000, 0.25 = 0x3400
    auto out = convert_fp16_raw_to_fp32(MakeRawFp16({0x3C00, 0x3800, 0xC000, 0x3400}));
    ASSERT_EQ(out.size(), 4u);
    EXPECT_FLOAT_EQ(out[0], 1.0f);
    EXPECT_FLOAT_EQ(out[1], 0.5f);
    EXPECT_FLOAT_EQ(out[2], -2.0f);
    EXPECT_FLOAT_EQ(out[3], 0.25f);
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32ZeroAndSign) {
    auto out = convert_fp16_raw_to_fp32(MakeRawFp16({0x0000, 0x8000}));
    ASSERT_EQ(out.size(), 2u);
    EXPECT_FLOAT_EQ(out[0], 0.0f);
    EXPECT_FALSE(std::signbit(out[0]));
    EXPECT_FLOAT_EQ(out[1], 0.0f);
    EXPECT_TRUE(std::signbit(out[1]));
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32MaxNormal) {
    // 65504 = 0x7BFF is the largest finite FP16 value
    auto out = convert_fp16_raw_to_fp32(MakeRawFp16({0x7BFF}));
    ASSERT_EQ(out.size(), 1u);
    EXPECT_FLOAT_EQ(out[0], 65504.0f);
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32Subnormal) {
    // Smallest subnormal 0x0001 = 2^-24, largest subnormal 0x03FF ~ 6.0976e-5
    auto out = convert_fp16_raw_to_fp32(MakeRawFp16({0x0001, 0x03FF}));
    ASSERT_EQ(out.size(), 2u);
    EXPECT_FLOAT_EQ(out[0], std::ldexp(1.0f, -24));
    EXPECT_NEAR(out[1], 6.0975552e-5f, 1e-11);
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32InfAndNaN) {
    auto out = convert_fp16_raw_to_fp32(MakeRawFp16({0x7C00, 0xFC00, 0x7E00}));
    ASSERT_EQ(out.size(), 3u);
    EXPECT_TRUE(std::isinf(out[0]));
    EXPECT_GT(out[0], 0.0f);
    EXPECT_TRUE(std::isinf(out[1]));
    EXPECT_LT(out[1], 0.0f);
    EXPECT_TRUE(std::isnan(out[2]));
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32ManyValues) {
    // Simulate a 512x512 FP16 initializer filled with 1.0 (0x3C00)
    std::string raw;
    raw.reserve(static_cast<size_t>(512) * 512 * 2);
    for (int i = 0; i < 512 * 512; ++i) {
        raw.push_back(static_cast<char>(0x00));
        raw.push_back(static_cast<char>(0x3C));
    }
    auto out = convert_fp16_raw_to_fp32(raw);
    ASSERT_EQ(out.size(), static_cast<size_t>(512) * 512);
    for (float v : out) { EXPECT_FLOAT_EQ(v, 1.0f); }
}

TEST_F(FaceHelperTest, ConvertFp16ToFp32EmptyInput) {
    auto out = convert_fp16_raw_to_fp32("");
    EXPECT_TRUE(out.empty());
}
