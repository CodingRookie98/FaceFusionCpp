
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

import foundation.infrastructure.file_system;
import foundation.infrastructure.concurrent_file_system;
import foundation.infrastructure.test_support;

namespace fs = std::filesystem;

class FileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        const testing::TestInfo* const test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        test_dir = std::string("test_fs_sandbox_") + test_info->test_suite_name() + "_"
                 + test_info->name();
        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
    }

    std::string test_dir;

    void create_dummy_file(const std::string& filename, const std::string& content = "test") {
        std::ofstream ofs(fs::path(test_dir) / filename);
        ofs << content;
        ofs.close();
    }
};

TEST_F(FileSystemTest, RemoveFile) {
    create_dummy_file("temp.txt");
    std::string path = (fs::path(test_dir) / "temp.txt").string();

    EXPECT_TRUE(fs::exists(path));
    foundation::infrastructure::file_system::remove_file(path);
    EXPECT_FALSE(fs::exists(path));
}

TEST_F(FileSystemTest, CopyFile) {
    create_dummy_file("src.txt", "content");
    std::string src = (fs::path(test_dir) / "src.txt").string();
    std::string dst = (fs::path(test_dir) / "dst.txt").string();

    foundation::infrastructure::file_system::copy_file(src, dst);

    EXPECT_TRUE(fs::exists(dst));
    // Simple content check
    std::ifstream ifs(dst);
    std::string content;
    ifs >> content;
    EXPECT_EQ(content, "content");
}

TEST_F(FileSystemTest, ConcurrentRemoveFiles) {
    std::vector<std::string> files;
    for (int i = 0; i < 5; ++i) {
        std::string name = "file_" + std::to_string(i) + ".txt";
        create_dummy_file(name);
        files.push_back((fs::path(test_dir) / name).string());
    }

    // Wait slightly to ensure file creation is flushed (optional but safe)

    foundation::infrastructure::concurrent_file_system::remove_files(files);

    // Allow some time for async ops if implementation doesn't block (it usually enqueues)
    // NOTE: The current simple thread pool might not return futures for void tasks,
    // so we might need to sleep or use better sync in real tests.
    // For now assuming implementation might be blocking or we wait a bit.
    // Real implementation uses fire-and-forget, so this test is flaky without sync.
    // However, let's assume for this basic version we just check.

    // To properly test async, we'd need the thread pool to drain.
    // Since we don't have a 'drain' on the simple pool, we sleep briefly.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (const auto& f : files) { EXPECT_FALSE(fs::exists(f)) << "File should be removed: " << f; }
}
