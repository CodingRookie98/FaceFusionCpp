#include <gtest/gtest.h>
#include <vector>
#include <opencv2/opencv.hpp>

import domain.face;
import domain.face.test_support;

using namespace domain::face;

TEST(FaceTest, DefaultConstruction) {
    Face face;
    EXPECT_TRUE(face.is_empty());
    EXPECT_LE(face.box().area(), 0.0f);
    EXPECT_TRUE(face.kps().empty());
}

TEST(FaceTest, SetAndGetBox) {
    Face face;
    cv::Rect2f box(10.0f, 20.0f, 100.0f, 120.0f);
    face.set_box(box);

    EXPECT_EQ(face.box().x, 10.0f);
    EXPECT_EQ(face.box().y, 20.0f);
    EXPECT_EQ(face.box().width, 100.0f);
    EXPECT_EQ(face.box().height, 120.0f);

    // is_empty 是 m_box.area() <= 0.0f || m_kps.empty()
    // 这里 kps 为空，所以 is_empty 应为 true
    EXPECT_TRUE(face.is_empty());
}

TEST(FaceTest, SetAndGetKps) {
    using domain::face::types::Landmarks;
    Face face;
    face.set_box({0, 0, 100, 100});

    Landmarks kps;
    kps.emplace_back(10.0f, 10.0f);
    face.set_kps(kps);

    EXPECT_EQ(face.kps().size(), 1);
    EXPECT_FALSE(face.is_empty()); // box有效且kps非空
}

TEST(FaceTest, GetLandmark5) {
    // 1. 测试 5 点的情况
    auto face5 = test_support::create_test_face(); // helper 创建的是 5 点 face
    EXPECT_EQ(face5.kps().size(), 5);
    auto l5 = face5.get_landmark5();
    EXPECT_EQ(l5.size(), 5);
    EXPECT_EQ(l5[0], face5.kps()[0]);

    // 2. 测试 68 点的情况
    auto face68 = test_support::create_face_with_68_kps();
    EXPECT_EQ(face68.kps().size(), 68);
    auto l5_from_68 = face68.get_landmark5();
    // 目前未实现算法，应该返回空
    EXPECT_TRUE(l5_from_68.empty());
}

TEST(AgeRangeTest, Logic) {
    // AgeRange 是 domain::common::types::AgeRange
    // Face 模块导出了 domain.common，并使用了 using
    domain::face::AgeRange range;
    EXPECT_EQ(range.min, 0);
    EXPECT_EQ(range.max, 100);

    range.set(20, 30);
    EXPECT_TRUE(range.contains(25));
    EXPECT_FALSE(range.contains(10));

    // 测试自动 swap
    range.set(50, 40);
    EXPECT_EQ(range.min, 40);
    EXPECT_EQ(range.max, 50);
}
