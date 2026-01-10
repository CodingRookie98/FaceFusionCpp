
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <nlohmann/json.hpp>

import foundation.infrastructure.core_utils;

using namespace foundation::infrastructure::core_utils;

TEST(CoreUtilsRandomTest, GenerateRandomStr) {
    std::string str1 = random::generate_random_str(10);
    std::string str2 = random::generate_random_str(10);

    EXPECT_EQ(str1.length(), 10);
    EXPECT_EQ(str2.length(), 10);
    EXPECT_NE(str1, str2);
}

TEST(CoreUtilsRandomTest, GenerateUUID) {
    std::string uuid1 = random::generate_uuid();
    std::string uuid2 = random::generate_uuid();

    EXPECT_EQ(uuid1.length(), 36);
    EXPECT_NE(uuid1, uuid2);
}

TEST(CoreUtilsConversionTest, YamlToJson) {
    std::string yaml_str = "pool: 5\nfiles:\n  - a.txt\n  - b.txt";
    nlohmann::json j = conversion::yaml_str_to_json(yaml_str);

    EXPECT_EQ(j["pool"], 5);
    EXPECT_TRUE(j["files"].is_array());
    EXPECT_EQ(j["files"].size(), 2);
    EXPECT_EQ(j["files"][0], "a.txt");
}

TEST(CoreUtilsConversionTest, JsonToYaml) {
    nlohmann::json j;
    j["name"] = "test";
    j["value"] = 123;

    std::string yaml_str = conversion::json_to_yaml_str(j);

    EXPECT_THAT(yaml_str, testing::HasSubstr("name: test"));
    EXPECT_THAT(yaml_str, testing::HasSubstr("value: 123"));
}
