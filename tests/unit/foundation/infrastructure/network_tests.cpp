
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <string>

import foundation.infrastructure.network;

namespace fs = std::filesystem;

class NetworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "test_network_sandbox";
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
         if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
         }
    }
    std::string test_dir;
};

TEST_F(NetworkTest, GetFileNameFromUrl) {
    using namespace foundation::infrastructure::network;

    EXPECT_EQ(get_file_name_from_url("http://example.com/file.txt"), "file.txt");
    EXPECT_EQ(get_file_name_from_url("http://example.com/file.txt?q=1"), "file.txt");
    EXPECT_EQ(get_file_name_from_url("http://example.com/"), "downloaded_file");
}

TEST_F(NetworkTest, HumanReadableSize) {
    using namespace foundation::infrastructure::network;

    EXPECT_EQ(human_readable_size(500), "500.00 B");
    EXPECT_EQ(human_readable_size(1024), "1.00 KB");
    EXPECT_EQ(human_readable_size(1024 * 1024 + 50000), "1.05 MB");
}

// NOTE: Download tests involve real network calls.
// For a unit test environment without mocking, we usually verify "failure" on invalid urls
// or skip real downloads to avoid flakiness.

TEST_F(NetworkTest, DownloadInvalidUrl) {
    using namespace foundation::infrastructure::network;

    // Expect failure (throw)
    EXPECT_THROW({
        download("http://invalid.url.that.does.not.exist/file.txt", test_dir);
    }, std::runtime_error);
    // Or whatever exception implementation throws, likely runtime_error from CURL code
}
