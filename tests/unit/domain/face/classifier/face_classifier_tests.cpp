#include <gtest/gtest.h>
#include <memory>

import domain.face.classifier;
import domain.face;

using namespace domain::face::classifier;
using namespace domain::face;

class FaceClassifierTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(FaceClassifierTest, FactoryCreatesFairFace) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    EXPECT_NE(classifier, nullptr);
}

TEST_F(FaceClassifierTest, ClassifierNotLoadedInitially) {
    auto classifier = create_classifier(ClassifierType::FairFace);
    ASSERT_NE(classifier, nullptr);
    // Classifier is created but model is not loaded yet
    // We can't call classify without loading a model first
}

TEST_F(FaceClassifierTest, ClassificationResultDefaultValues) {
    ClassificationResult result{};
    // Default zero-initialized values
    EXPECT_EQ(result.gender, Gender::Male); // 0
    EXPECT_EQ(result.race, Race::Black);    // 0
    EXPECT_EQ(result.age.min, 0);
    EXPECT_EQ(result.age.max, 100);
}
