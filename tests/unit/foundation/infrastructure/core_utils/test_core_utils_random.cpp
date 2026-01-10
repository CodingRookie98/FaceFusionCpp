import foundation.infrastructure.core_utils;

#include <gtest/gtest.h>
#include <unordered_set>

class CoreUtilsRandomTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CoreUtilsRandomTest, GenerateRandomStr_NormalCase) {
    const size_t length = 10;
    auto result = foundation::infrastructure::core_utils::random::generate_random_str(length);

    EXPECT_EQ(result.length(), length);
    for (char c : result) {
        EXPECT_TRUE(std::isalnum(c));
    }
}

TEST_F(CoreUtilsRandomTest, GenerateRandomStr_LengthOne) {
    const size_t length = 1;
    auto result = foundation::infrastructure::core_utils::random::generate_random_str(length);

    EXPECT_EQ(result.length(), length);
    EXPECT_TRUE(std::isalnum(result[0]));
}

TEST_F(CoreUtilsRandomTest, GenerateRandomStr_LengthZero) {
    const size_t length = 0;

    EXPECT_THROW(
        foundation::infrastructure::core_utils::random::generate_random_str(length),
        std::invalid_argument);
}

TEST_F(CoreUtilsRandomTest, GenerateRandomStr_LargeLength) {
    const size_t length = 1000;
    auto result = foundation::infrastructure::core_utils::random::generate_random_str(length);

    EXPECT_EQ(result.length(), length);
    for (char c : result) {
        EXPECT_TRUE(std::isalnum(c));
    }
}

TEST_F(CoreUtilsRandomTest, GenerateRandomStr_Uniqueness) {
    const size_t length = 20;
    const size_t iterations = 100;

    std::unordered_set<std::string> unique_strings;

    for (size_t i = 0; i < iterations; ++i) {
        auto result = foundation::infrastructure::core_utils::random::generate_random_str(length);
        unique_strings.insert(result);
    }

    EXPECT_GT(unique_strings.size(), iterations * 0.95);
}

TEST_F(CoreUtilsRandomTest, GenerateUUID_Format) {
    auto uuid = foundation::infrastructure::core_utils::random::generate_uuid();

    EXPECT_EQ(uuid.length(), 36);
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');
}

TEST_F(CoreUtilsRandomTest, GenerateUUID_Uniqueness) {
    const size_t iterations = 100;
    std::unordered_set<std::string> unique_uuids;

    for (size_t i = 0; i < iterations; ++i) {
        auto uuid = foundation::infrastructure::core_utils::random::generate_uuid();
        unique_uuids.insert(uuid);
    }

    EXPECT_EQ(unique_uuids.size(), iterations);
}

TEST_F(CoreUtilsRandomTest, GenerateUUID_Version4) {
    auto uuid = foundation::infrastructure::core_utils::random::generate_uuid();

    EXPECT_EQ(uuid[14], '4');
}

TEST_F(CoreUtilsRandomTest, GenerateUUID_Variant) {
    auto uuid = foundation::infrastructure::core_utils::random::generate_uuid();

    char variant_char = uuid[19];
    EXPECT_TRUE(variant_char == '8' || variant_char == '9' || variant_char == 'a' || variant_char == 'b');
}
