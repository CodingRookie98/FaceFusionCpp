#include <gtest/gtest.h>
#include <memory>

import domain.face.enhancer;

using namespace domain::face::enhancer;

TEST(FaceEnhancerFactoryTest, CreateCodeFormer) {
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::CodeFormer);
    EXPECT_NE(enhancer, nullptr);
}

TEST(FaceEnhancerFactoryTest, CreateGfpGan) {
    auto enhancer = FaceEnhancerFactory::create(FaceEnhancerFactory::Type::GfpGan);
    EXPECT_NE(enhancer, nullptr);
}
