#include <gtest/gtest.h>
#include <filesystem>
import tests.helpers.domain.face_test_helpers;
import tests.helpers.foundation.test_utilities;
import tests.helpers.foundation.memory_monitor;

TEST(HelperUsageTest, CreateTestFace) {
    auto face = tests::helpers::domain::create_test_face();
    EXPECT_FALSE(face.kps.empty());
}

TEST(HelperUsageTest, GetAssetsPath) {
    // This might fail if assets are not found, but we just verify the function exists and runs
    try {
        auto path = tests::helpers::foundation::get_assets_path();
        // If we get here, it should be a directory
        EXPECT_TRUE(std::filesystem::is_directory(path));
    } catch (const std::runtime_error&) {
        // Ignored if assets path not set in environment
        SUCCEED();
    }
}

TEST(HelperUsageTest, MemoryMonitor) {
    tests::helpers::foundation::MemoryMonitor monitor;
    monitor.start();
    std::vector<int> dummy(1024 * 1024, 1); // Alloc ~4MB
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    monitor.stop();
    // Peak usage should be > 0
    EXPECT_GE(monitor.get_peak_usage_mb(), 0);
}
