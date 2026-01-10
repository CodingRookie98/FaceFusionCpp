module;

#include <string>
#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

export module foundation.infrastructure.core_utils;

export namespace foundation::infrastructure::core_utils {

namespace random {

/**
 * @brief Generate a random string of specified length
 * @param length The length of the random string to generate
 * @return std::string Random string containing alphanumeric characters
 * @throws std::invalid_argument if length is zero
 * @note Thread-safe (uses static local variables for thread-safe initialization)
 * @exception-safe Strong guarantee
 */
std::string generate_random_str(const size_t& length);

/**
 * @brief Generate a UUID (Universally Unique Identifier) version 4
 * @return std::string UUID string in standard 8-4-4-4-12 format
 * @note Thread-safe (uses static local variables for thread-safe initialization)
 * @exception-safe Strong guarantee
 */
std::string generate_uuid();

} // namespace random

namespace conversion {

/**
 * @brief Convert YAML string to JSON object
 * @param yaml_str The YAML string to convert
 * @return nlohmann::json JSON object representing the YAML content
 * @throws std::runtime_error if YAML parsing fails
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
nlohmann::json yaml_str_to_json(const std::string& yaml_str);

/**
 * @brief Convert JSON object to YAML string
 * @param j The JSON object to convert
 * @return std::string YAML string representation
 * @throws std::runtime_error if JSON to YAML conversion fails
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
std::string json_to_yaml_str(const nlohmann::json& j);

/**
 * @brief Convert YAML node to JSON object recursively
 * @param node The YAML node to convert
 * @return nlohmann::json JSON object representing the YAML node
 * @throws std::runtime_error if YAML node type is unsupported
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
nlohmann::json yaml_node_to_json(const YAML::Node& node);

/**
 * @brief Convert JSON object to YAML node recursively
 * @param j The JSON object to convert
 * @return YAML::Node YAML node representing the JSON object
 * @throws std::runtime_error if JSON type is unsupported
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 */
YAML::Node json_to_yaml_node(const nlohmann::json& j);

} // namespace conversion

} // namespace foundation::infrastructure::core_utils
