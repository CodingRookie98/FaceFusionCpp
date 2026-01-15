
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <string>
#include <fstream>

import foundation.infrastructure.network;

namespace fs = std::filesystem;

class NetworkTest : public ::testing::Test {
protected:
    void SetUp() override {
        const testing::TestInfo* const test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        test_dir = std::string("test_network_sandbox_") + test_info->test_suite_name() + "_"
                 + test_info->name();
        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
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
    EXPECT_THROW(
        { download("http://invalid.url.that.does.not.exist/file.txt", test_dir); },
        std::runtime_error);
    // Or whatever exception implementation throws, likely runtime_error from CURL code
}

TEST_F(NetworkTest, DownloadLocalFile) {
    using namespace foundation::infrastructure::network;

    // Create a dummy source file
    std::string source_filename = "test_source.txt";
    fs::path source_path = fs::absolute(fs::path(test_dir) / source_filename);

    // Ensure parent dir exists (it should, SetUp created test_dir)
    // But we need to create the file content
    {
        std::ofstream ofs(source_path);
        ofs << "Hello World Content";
        ofs.close();
    }

    // Target output directory (inside test_dir, but maybe a subdir to avoid conflict if we were
    // downloading to same dir) Actually, download() takes output_dir. source_path is in test_dir.
    // Let's create a "downloads" subdir.
    fs::path download_dir = fs::path(test_dir) / "downloads";
    fs::create_directories(download_dir);

    // Construct file:// URL
    // On Windows, file:///C:/path...
    std::string url = "file:///" + source_path.generic_string();

    // Perform download
    EXPECT_NO_THROW({
        bool result = download(url, download_dir.string());
        EXPECT_TRUE(result);
    });

    // Verify final file exists
    fs::path expected_output = download_dir / source_filename;
    EXPECT_TRUE(fs::exists(expected_output));

    // Verify content matches
    std::ifstream ifs(expected_output);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    EXPECT_EQ(content, "Hello World Content");

    // Verify .tmp file is gone
    fs::path tmp_output = expected_output;
    tmp_output += ".tmp";
    EXPECT_FALSE(fs::exists(tmp_output));
}
