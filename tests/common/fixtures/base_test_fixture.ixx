module;
#include <gtest/gtest.h>
#include <string>

export module tests.common.fixtures.base_test_fixture;

export namespace tests::common::fixtures {
class BaseTestFixture : public ::testing::Test {
protected:
    std::string GetTestName() const;
};
} // namespace tests::common::fixtures
