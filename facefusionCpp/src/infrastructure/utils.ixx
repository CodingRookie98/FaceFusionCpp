/**
 * @file utils.ixx
 * @brief Utility functions module
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides common utility functions for the application
 */
module;
#include <random>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

export module utils;
import face;

namespace ffc::infra::utils {

using json = nlohmann::json;

/**
 * @brief Convert YAML string to JSON object
 * @param yaml_str YAML formatted string
 * @return nlohmann::json JSON object converted from YAML
 * @note This function uses yaml-cpp library for YAML parsing
 */
export nlohmann::json yaml_str_to_json(const std::string& yaml_str);

/**
 * @brief Convert YAML node to JSON object
 * @param node YAML node object
 * @return nlohmann::json JSON object converted from YAML node
 * @note This function uses yaml-cpp library for YAML parsing
 */
export nlohmann::json yaml_node_to_json(const YAML::Node& node);

/**
 * @brief Convert JSON object to YAML string
 * @param j JSON object to convert
 * @return std::string YAML formatted string
 * @note This function uses yaml-cpp library for YAML generation
 */
export std::string json_to_yaml_str(const nlohmann::json& j);

/**
 * @brief Convert JSON object to YAML node
 * @param j JSON object to convert
 * @return YAML::Node YAML node object
 * @note This function uses yaml-cpp library for YAML generation
 */
export YAML::Node json_to_yaml_node(const nlohmann::json& j);

/**
 * @brief Load YAML file and parse to YAML node
 * @param yaml_file_path Path to the YAML file
 * @return YAML::Node Parsed YAML node
 * @throws std::runtime_error if file cannot be loaded or parsed
 */
export YAML::Node load_yaml_file(const std::string& yaml_file_path);

/**
 * @brief Load JSON file and parse to JSON object
 * @param json_file_path Path to the JSON file
 * @return json Parsed JSON object
 * @throws std::runtime_error if file cannot be loaded or parsed
 */
export json load_json_file(const std::string& json_file_path);

/**
 * @brief Save JSON object to file
 * @param json_file_path Path to save the JSON file
 * @param j JSON object to save
 * @param indent Indentation level for pretty printing (default: 2)
 * @return bool True if save was successful, false otherwise
 */
export bool save_json_file(const std::string& json_file_path, const json& j, int indent = 2);

/**
 * @brief Load YAML file and convert to JSON object
 * @param yaml_file_path Path to the YAML file
 * @return json JSON object converted from YAML file
 * @note This function combines load_yaml_file and yaml_node_to_json
 */
export json yaml_file_to_json(const std::string& yaml_file_path);

/**
 * @brief Generate a random string of specified length
 * @param length Length of the random string to generate
 * @return std::string Random string containing alphanumeric characters
 * @note Uses std::random_device and std::mt19937 for random number generation
 */
export std::string generate_random_str(const size_t& length);

/**
 * @brief Generate a UUID (Universally Unique Identifier)
 * @return std::string UUID string in standard format
 * @note Uses std::random_device for entropy source
 */
export std::string generate_uuid();

/**
 * @brief Get all values of an enum type
 * @tparam EnumType Enum type
 * @return std::initializer_list<EnumType> List containing all enum values
 * @note This template function requires specialization for each enum type
 * @warning This function will throw static_assert if not specialized
 */
export template <typename EnumType>
    requires std::is_enum_v<EnumType>
constexpr std::initializer_list<EnumType> enum_all() {
    static_assert(false, "Please provide specialization for enum_all<EnumType>");
    return std::initializer_list<EnumType>();
}

/**
 * @brief Specialization for Gender enum
 * @return std::initializer_list<Gender> All Gender enum values
 */
export template <>
constexpr std::initializer_list<Gender> enum_all<Gender>();

/**
 * @brief Specialization for Race enum
 * @return std::initializer_list<Race> All Race enum values
 */
export template <>
constexpr std::initializer_list<Race> enum_all<Race>();
} // namespace ffc::infra::utils