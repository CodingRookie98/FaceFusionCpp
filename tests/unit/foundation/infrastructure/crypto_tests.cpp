
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>

import foundation.infrastructure.crypto;

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
