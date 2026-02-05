#include <gtest/gtest.h>
#include <memory>

import domain.face.landmarker;

using namespace domain::face::landmarker;

TEST(FaceLandmarkerFactoryTest, CreateT2dfan) {
    auto landmarker = create_landmarker(LandmarkerType::T2dfan);
    EXPECT_NE(landmarker, nullptr);
}

TEST(FaceLandmarkerFactoryTest, CreatePeppawutz) {
    auto landmarker = create_landmarker(LandmarkerType::Peppawutz);
    EXPECT_NE(landmarker, nullptr);
}

TEST(FaceLandmarkerFactoryTest, CreateT68By5) {
    auto landmarker = create_landmarker(LandmarkerType::T68By5);
    EXPECT_NE(landmarker, nullptr);
}
