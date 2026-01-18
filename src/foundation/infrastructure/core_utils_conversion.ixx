module;
#include <nlohmann/json.hpp>
#include <string>

/**
 * @file core_utils_conversion.ixx
 * @brief Core utility module partition for conversion utilities
 * @author CodingRookie
 * @date 2026-01-18
 */
export module foundation.infrastructure.core_utils:conversion;

namespace foundation::infrastructure::core_utils::conversion {

/**
 * @brief Convert YAML string to JSON object
 * @param yaml_str The YAML string to convert
 * @return JSON object
 */
export nlohmann::json yaml_str_to_json(const std::string& yaml_str);

/**
 * @brief Convert JSON object to YAML string
 * @param j The JSON object to convert
 * @return YAML string
 */
export std::string json_to_yaml_str(const nlohmann::json& j);
} // namespace foundation::infrastructure::core_utils::conversion
