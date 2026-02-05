#include <gtest/gtest.h>
#include <memory>

import domain.face.detector;

using namespace domain::face::detector;

TEST(FaceDetectorFactoryTest, CreateYoloDetector) {
    auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
    EXPECT_NE(detector, nullptr);
}

TEST(FaceDetectorFactoryTest, CreateScrfdDetector) {
    auto detector = FaceDetectorFactory::create(DetectorType::SCRFD);
    EXPECT_NE(detector, nullptr);
}

TEST(FaceDetectorFactoryTest, CreateRetinaFaceDetector) {
    auto detector = FaceDetectorFactory::create(DetectorType::RetinaFace);
    EXPECT_NE(detector, nullptr);
}
