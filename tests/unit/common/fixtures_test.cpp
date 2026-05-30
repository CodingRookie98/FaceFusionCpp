1 | #include < gtest / gtest.h > 2 | import tests.common.fixtures.base_test_fixture;
3 | 4 | class ConcreteTestFixture : public tests::common::fixtures::BaseTestFixture {};
5 | 6 | TEST_F(ConcreteTestFixture, GetTestNameReturnsCorrectName) {
    7 | // Assert
        8 | EXPECT_EQ(GetTestName(), "GetTestName_ReturnsCorrectName");
    9 |
}
10 |