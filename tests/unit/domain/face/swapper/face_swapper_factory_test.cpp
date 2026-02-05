#include <gtest/gtest.h>
#include <memory>

import domain.face.swapper;

using namespace domain::face::swapper;

TEST(FaceSwapperFactoryTest, CreateInSwapper) {
    auto swapper = FaceSwapperFactory::create_inswapper();
    EXPECT_NE(swapper, nullptr);
    // Basic check to ensure it's initialized correctly (impl base constructor called)
    // We can't easily check internal state without friends or accessors, but non-null is good start.
}
