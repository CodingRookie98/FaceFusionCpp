module;
#include <gtest/gtest.h>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include "common/fixtures/global_env_helper.h"

module tests.common.fixtures.integration_test_fixture;

namespace tests::common::fixtures {
void IntegrationTestFixture::SetUpTestSuite() {
    // Use helper to link global environment
    // Crucial: Must use the helper from plain C++ file to avoid module mangling of the extern
    // symbol
    details::LinkGlobalEnvHelper();
}

void IntegrationTestFixture::SetUp() {
    BaseTestFixture::SetUp();
}

void IntegrationTestFixture::TearDown() {
    BaseTestFixture::TearDown();
}

std::filesystem::path IntegrationTestFixture::GetAssetsPath() const {
    if (const char* env_p = std::getenv("FACEFUSION_ASSETS_PATH")) {
        std::filesystem::path path(env_p);
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            return std::filesystem::absolute(path);
        }
    }

    std::filesystem::path current = std::filesystem::current_path();
    int max_depth = 10;

    while (max_depth-- > 0) {
        std::filesystem::path potential = current / "assets";
        if (std::filesystem::exists(potential) && std::filesystem::is_directory(potential)
            && std::filesystem::exists(potential / "models_info.json")) {
            return std::filesystem::absolute(potential);
        }

        if (!current.has_parent_path() || current == current.parent_path()) { break; }
        current = current.parent_path();
    }

    throw std::runtime_error("Could not find assets directory.");
}

std::filesystem::path IntegrationTestFixture::GetTestDataPath(
    const std::string& relative_path) const {
    return GetAssetsPath() / relative_path;
}
} // namespace tests::common::fixtures
