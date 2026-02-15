export module tests.common.fixtures.unit_test_fixture;

import tests.common.fixtures.base_test_fixture;

export namespace tests::common::fixtures {
class UnitTestFixture : public BaseTestFixture {
protected:
    void SetUp() override;
    void TearDown() override;
};
} // namespace tests::common::fixtures
