#include <gtest/gtest.h>
#include "common/test_paths.h"
#include <filesystem>
#include <fstream>

namespace tests::unit::common {

class TestPathsTest : public ::testing::Test {};

TEST_F(TestPathsTest, GetExecutableDirReturnsValidPath) {
    auto path = tests::common::TestPaths::GetExecutableDir();
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(TestPathsTest, GetTestOutputDirCreatesDirectory) {
    std::string category = "unit_test_check_" + std::to_string(std::time(nullptr));
    auto output_dir = tests::common::TestPaths::GetTestOutputDir(category);

    EXPECT_TRUE(std::filesystem::exists(output_dir));
    EXPECT_TRUE(std::filesystem::is_directory(output_dir));

    // Check if path structure is correct (ends with category)
    EXPECT_EQ(output_dir.filename().string(), category);

    // Test writing a file there
    auto test_file = output_dir / "test.txt";
    std::ofstream ofs(test_file);
    ofs << "test";
    ofs.close();

    EXPECT_TRUE(std::filesystem::exists(test_file));

    // Clean up
    std::filesystem::remove_all(output_dir);
}

} // namespace tests::unit::common
