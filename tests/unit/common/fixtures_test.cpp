#include <gtest/gtest.h>
import tests.common.fixtures.base_test_fixture;

class ConcreteTestFixture : public tests::common::fixtures::BaseTestFixture {};

TEST_F(ConcreteTestFixture, GetTestNameReturnsCorrectName) {
    // Assert
    EXPECT_EQ(GetTestName(), "GetTestNameReturnsCorrectName");
}
