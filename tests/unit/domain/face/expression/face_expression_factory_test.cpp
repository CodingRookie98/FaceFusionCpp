#include <gtest/gtest.h>
#include <memory>

import domain.face.expression;

using namespace domain::face::expression;

TEST(FaceExpressionFactoryTest, CreateLivePortraitRestorer) {
    auto restorer = create_live_portrait_restorer();
    EXPECT_NE(restorer, nullptr);
}
