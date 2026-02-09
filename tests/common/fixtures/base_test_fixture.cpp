module;
#include <gtest/gtest.h>
#include <string>

module tests.common.fixtures.base_test_fixture;

namespace tests::common::fixtures {
std::string BaseTestFixture::GetTestName() const {
    const ::testing::TestInfo* const test_info =
        ::testing::UnitTest::GetInstance()->current_test_info();
    if (test_info) return test_info->name();
    return "UnknownTest";
}
} // namespace tests::common::fixtures
