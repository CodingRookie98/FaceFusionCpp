module tests.common.fixtures.unit_test_fixture;

namespace tests::common::fixtures {
void UnitTestFixture::SetUp() {
    BaseTestFixture::SetUp();
}

void UnitTestFixture::TearDown() {
    BaseTestFixture::TearDown();
}
} // namespace tests::common::fixtures
