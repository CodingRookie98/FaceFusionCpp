#include <gtest/gtest.h>
#include <memory>

import domain.face.recognizer;

using namespace domain::face::recognizer;

TEST(FaceRecognizerFactoryTest, CreateArcFace) {
    auto recognizer = create_face_recognizer(FaceRecognizerType::ArcFaceW600kR50);
    EXPECT_NE(recognizer, nullptr);
}
