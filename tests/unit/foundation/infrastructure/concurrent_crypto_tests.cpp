
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <gmock/gmock.h>

import foundation.infrastructure.concurrent_crypto;

namespace fs = std::filesystem;

class ConcurrentCryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        const testing::TestInfo* const test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        test_dir = std::string("test_ccrypto_sandbox_") + test_info->test_suite_name() + "_"
                 + test_info->name();

        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
    }

    std::string test_dir;

    std::string create_dummy_file(const std::string& filename, const std::string& content) {
        std::string path = (fs::path(test_dir) / filename).string();
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        return path;
    }
};

TEST_F(ConcurrentCryptoTest, Sha1BatchAsync) {
    std::string p1 = create_dummy_file("f1.txt", "test");
    std::string p2 = create_dummy_file("f2.txt", "hello");
    // "test" sha1 = a94a8fe5ccb19ba61c4c0873d391e987982fbbd3
    // "hello" sha1 = aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d

    std::unordered_set<std::string> files = {p1, p2};
    auto results = foundation::infrastructure::concurrent_crypto::sha1_batch(files);

    bool found_test = false;
    bool found_hello = false;

    for (const auto& h : results) {
        if (h == "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3") found_test = true;
        if (h == "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d") found_hello = true;
    }

    EXPECT_TRUE(found_test);
    EXPECT_TRUE(found_hello);
}

TEST_F(ConcurrentCryptoTest, CombinedSha1) {
    std::string p1 = create_dummy_file("f1.txt", "test");
    std::string p2 = create_dummy_file("f2.txt", "hello");
    // combined logic depends on implementation. usually sorts hashes and hashes string.
    // We assume deterministic output.

    std::unordered_set<std::string> files = {p1, p2};
    std::string combined_hash = foundation::infrastructure::concurrent_crypto::combined_sha1(files);

    EXPECT_FALSE(combined_hash.empty());
    EXPECT_EQ(combined_hash.length(), 40); // SHA1 hex length

    // Verify consistency
    std::string combined_hash2 =
        foundation::infrastructure::concurrent_crypto::combined_sha1(files);
    EXPECT_EQ(combined_hash, combined_hash2);
}
