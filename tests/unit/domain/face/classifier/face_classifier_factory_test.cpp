#include <gtest/gtest.h>
#include <memory>

import domain.face.classifier;

using namespace domain::face::classifier;

TEST(FaceClassifierFactoryTest, CreateFairFace) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    EXPECT_NE(classifier, nullptr);
}
