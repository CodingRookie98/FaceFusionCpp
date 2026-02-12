#pragma once

#include <string>
#include <filesystem>
#include <gtest/gtest.h>

namespace tests::common {

class TestPaths {
public:
    static std::filesystem::path GetExecutableDir();
    static std::filesystem::path GetTestOutputDir(const std::string& category);
};

} // namespace tests::common
