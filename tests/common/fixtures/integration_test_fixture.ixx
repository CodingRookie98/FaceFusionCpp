module;
#include <string>
#include <filesystem>

export module tests.common.fixtures.integration_test_fixture;

import tests.common.fixtures.base_test_fixture;

export namespace tests::common::fixtures {
class IntegrationTestFixture : public BaseTestFixture {
public:
    static void SetUpTestSuite();

protected:
    void SetUp() override;
    void TearDown() override;

    std::filesystem::path GetAssetsPath() const;
    std::filesystem::path GetTestDataPath(const std::string& relative_path) const;
};
} // namespace tests::common::fixtures
