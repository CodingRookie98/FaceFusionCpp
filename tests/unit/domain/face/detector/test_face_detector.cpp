#include <gtest/gtest.h>
import domain.face.detector;

using namespace domain::face::detector;

TEST(FaceDetectorFactoryTest, CreateYolo) {
    auto detector = FaceDetectorFactory::create(DetectorType::Yolo);
    EXPECT_NE(detector, nullptr);
}

TEST(FaceDetectorFactoryTest, CreateOthers) {
    auto detector_scrfd = FaceDetectorFactory::create(DetectorType::SCRFD);
    EXPECT_NE(detector_scrfd, nullptr);

    auto detector_retina = FaceDetectorFactory::create(DetectorType::RetinaFace);
    EXPECT_NE(detector_retina, nullptr);
}
