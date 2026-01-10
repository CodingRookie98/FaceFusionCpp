import foundation.infrastructure.core_utils;

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class CoreUtilsConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CoreUtilsConversionTest, YamlStrToJson_SimpleString) {
    std::string yaml_str = "key: value";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("key"));
    EXPECT_EQ(json["key"], "value");
}

TEST_F(CoreUtilsConversionTest, YamlStrToJson_Integer) {
    std::string yaml_str = "number: 42";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("number"));
    EXPECT_EQ(json["number"], 42);
}

TEST_F(CoreUtilsConversionTest, YamlStrToJson_Float) {
    std::string yaml_str = "pi: 3.14159";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("pi"));
    EXPECT_NEAR(json["pi"].get<double>(), 3.14159, 0.00001);
}

TEST_F(CoreUtilsConversionTest, YamlStrToJson_Boolean) {
    std::string yaml_str = "enabled: true\ndisabled: false";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("enabled"));
    EXPECT_TRUE(json.contains("disabled"));
    EXPECT_TRUE(json["enabled"].get<bool>());
    EXPECT_FALSE(json["disabled"].get<bool>());
}

TEST_F(CoreUtilsConversionTest, YamlStrToList_Array) {
    std::string yaml_str = "items:\n  - apple\n  - banana\n  - cherry";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("items"));
    EXPECT_TRUE(json["items"].is_array());
    EXPECT_EQ(json["items"].size(), 3);
    EXPECT_EQ(json["items"][0], "apple");
    EXPECT_EQ(json["items"][1], "banana");
    EXPECT_EQ(json["items"][2], "cherry");
}

TEST_F(CoreUtilsConversionTest, YamlStrToJson_NestedObject) {
    std::string yaml_str = "person:\n  name: John\n  age: 30\n  address:\n    city: New York\n    zip: 10001";
    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(yaml_str);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("person"));
    EXPECT_TRUE(json["person"].is_object());
    EXPECT_TRUE(json["person"]["address"].is_object());
    EXPECT_EQ(json["person"]["name"], "John");
    EXPECT_EQ(json["person"]["age"], 30);
    EXPECT_EQ(json["person"]["address"]["city"], "New York");
    EXPECT_EQ(json["person"]["address"]["zip"], 10001);
}

TEST_F(CoreUtilsConversionTest, YamlStrToJson_InvalidYaml) {
    std::string invalid_yaml = "invalid: yaml: content:";

    EXPECT_THROW(
        foundation::infrastructure::core_utils::conversion::yaml_str_to_json(invalid_yaml),
        std::runtime_error);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlStr_SimpleString) {
    nlohmann::json json = {{"key", "value"}};
    auto yaml_str = foundation::infrastructure::core_utils::conversion::json_to_yaml_str(json);

    EXPECT_FALSE(yaml_str.empty());
    EXPECT_TRUE(yaml_str.find("key") != std::string::npos);
    EXPECT_TRUE(yaml_str.find("value") != std::string::npos);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlStr_Integer) {
    nlohmann::json json = {{"number", 42}};
    auto yaml_str = foundation::infrastructure::core_utils::conversion::json_to_yaml_str(json);

    EXPECT_FALSE(yaml_str.empty());
    EXPECT_TRUE(yaml_str.find("number") != std::string::npos);
    EXPECT_TRUE(yaml_str.find("42") != std::string::npos);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlStr_Array) {
    nlohmann::json json = {{"items", nlohmann::json::array({"apple", "banana", "cherry"})}};
    auto yaml_str = foundation::infrastructure::core_utils::conversion::json_to_yaml_str(json);

    EXPECT_FALSE(yaml_str.empty());
    EXPECT_TRUE(yaml_str.find("items") != std::string::npos);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlStr_NestedObject) {
    nlohmann::json json = {
        {"person", {{"name", "John"}, {"age", 30}, {"address", {{"city", "New York"}, {"zip", 10001}}}}}};
    auto yaml_str = foundation::infrastructure::core_utils::conversion::json_to_yaml_str(json);

    EXPECT_FALSE(yaml_str.empty());
    EXPECT_TRUE(yaml_str.find("person") != std::string::npos);
    EXPECT_TRUE(yaml_str.find("name") != std::string::npos);
    EXPECT_TRUE(yaml_str.find("address") != std::string::npos);
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_ScalarString) {
    YAML::Node node = YAML::Load("value");
    auto json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node);

    EXPECT_TRUE(json.is_string());
    EXPECT_EQ(json, "value");
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_ScalarInteger) {
    YAML::Node node = YAML::Load("42");
    auto json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node);

    EXPECT_TRUE(json.is_number_integer());
    EXPECT_EQ(json, 42);
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_ScalarFloat) {
    YAML::Node node = YAML::Load("3.14");
    auto json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node);

    EXPECT_TRUE(json.is_number());
    EXPECT_NEAR(json.get<double>(), 3.14, 0.001);
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_ScalarBoolean) {
    YAML::Node node_true = YAML::Load("true");
    YAML::Node node_false = YAML::Load("false");

    auto json_true = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node_true);
    auto json_false = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node_false);

    EXPECT_TRUE(json_true.is_boolean());
    EXPECT_TRUE(json_false.is_boolean());
    EXPECT_TRUE(json_true.get<bool>());
    EXPECT_FALSE(json_false.get<bool>());
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_Sequence) {
    YAML::Node node = YAML::Load("[apple, banana, cherry]");
    auto json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node);

    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.size(), 3);
    EXPECT_EQ(json[0], "apple");
    EXPECT_EQ(json[1], "banana");
    EXPECT_EQ(json[2], "cherry");
}

TEST_F(CoreUtilsConversionTest, YamlNodeToJson_Map) {
    YAML::Node node = YAML::Load("{name: John, age: 30}");
    auto json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(node);

    EXPECT_TRUE(json.is_object());
    EXPECT_TRUE(json.contains("name"));
    EXPECT_TRUE(json.contains("age"));
    EXPECT_EQ(json["name"], "John");
    EXPECT_EQ(json["age"], 30);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_ScalarString) {
    nlohmann::json json = "value";
    auto node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json);

    EXPECT_TRUE(node.IsScalar());
    EXPECT_EQ(node.as<std::string>(), "value");
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_ScalarInteger) {
    nlohmann::json json = 42;
    auto node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json);

    EXPECT_TRUE(node.IsScalar());
    EXPECT_EQ(node.as<int>(), 42);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_ScalarFloat) {
    nlohmann::json json = 3.14;
    auto node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json);

    EXPECT_TRUE(node.IsScalar());
    EXPECT_NEAR(node.as<double>(), 3.14, 0.001);
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_ScalarBoolean) {
    nlohmann::json json_true = true;
    nlohmann::json json_false = false;

    auto node_true = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json_true);
    auto node_false = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json_false);

    EXPECT_TRUE(node_true.IsScalar());
    EXPECT_TRUE(node_false.IsScalar());
    EXPECT_TRUE(node_true.as<bool>());
    EXPECT_FALSE(node_false.as<bool>());
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_Array) {
    nlohmann::json json = nlohmann::json::array({"apple", "banana", "cherry"});
    auto node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json);

    EXPECT_TRUE(node.IsSequence());
    EXPECT_EQ(node.size(), 3);
    EXPECT_EQ(node[0].as<std::string>(), "apple");
    EXPECT_EQ(node[1].as<std::string>(), "banana");
    EXPECT_EQ(node[2].as<std::string>(), "cherry");
}

TEST_F(CoreUtilsConversionTest, JsonToYamlNode_Object) {
    nlohmann::json json = {{"name", "John"}, {"age", 30}};
    auto node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(json);

    EXPECT_TRUE(node.IsMap());
    EXPECT_EQ(node.size(), 2);
    EXPECT_EQ(node["name"].as<std::string>(), "John");
    EXPECT_EQ(node["age"].as<int>(), 30);
}

TEST_F(CoreUtilsConversionTest, RoundTrip_YamlToJsonToYaml) {
    std::string original_yaml = R"(
name: John Doe
age: 30
address:
  city: New York
  zip: 10001
hobbies:
  - reading
  - swimming
  - programming
)";

    auto json = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(original_yaml);
    auto result_yaml = foundation::infrastructure::core_utils::conversion::json_to_yaml_str(json);

    auto json2 = foundation::infrastructure::core_utils::conversion::yaml_str_to_json(result_yaml);

    EXPECT_EQ(json, json2);
}

TEST_F(CoreUtilsConversionTest, RoundTrip_JsonToYamlToJson) {
    nlohmann::json original_json = {
        {"name", "John Doe"},
        {"age", 30},
        {"address", {{"city", "New York"}, {"zip", 10001}}},
        {"hobbies", {"reading", "swimming", "programming"}}};

    auto yaml_node = foundation::infrastructure::core_utils::conversion::json_to_yaml_node(original_json);
    auto result_json = foundation::infrastructure::core_utils::conversion::yaml_node_to_json(yaml_node);

    EXPECT_EQ(original_json, result_json);
}
