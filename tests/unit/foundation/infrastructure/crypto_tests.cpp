
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>

import foundation.infrastructure.crypto;
import foundation.infrastructure.concurrent_crypto;

namespace fs = std::filesystem;

class CryptoTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = "test_crypto_sandbox";
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

    std::string create_dummy_file(const std::string& filename, const std::string& content) {
        std::string path = (fs::path(test_dir) / filename).string();
        std::ofstream ofs(path);
        ofs << content;
        ofs.close();
        return path;
    }
};

TEST_F(CryptoTest, Sha1Sync) {
    // "test" sha1 = a94a8fe5ccb19ba61c4c0873d391e987982fbbd3
    std::string path = create_dummy_file("test.txt", "test");

    std::string hash = foundation::infrastructure::crypto::sha1(path);
    EXPECT_EQ(hash, "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3");
}

TEST_F(CryptoTest, Sha1BatchAsync) {
    std::string p1 = create_dummy_file("f1.txt", "test");
    std::string p2 = create_dummy_file("f2.txt", "hello");
    // "hello" sha1 = aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d

    std::unordered_set<std::string> files = {p1, p2};
    auto results = foundation::infrastructure::concurrent_crypto::sha1_batch(files);

    // Results are sorted by path, so order is deterministic
    // f1.txt (test) should be first or second depending on path sort

    // We just check if both hashes are present
    bool found_test = false;
    bool found_hello = false;

    for(const auto& h : results) {
        if (h == "a94a8fe5ccb19ba61c4c0873d391e987982fbbd3") found_test = true;
        if (h == "aaf4c61ddcc5e8a2dabede0f3b482cd9aea9434d") found_hello = true;
    }

    EXPECT_TRUE(found_test);
    EXPECT_TRUE(found_hello);
}
