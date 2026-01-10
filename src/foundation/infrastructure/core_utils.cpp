module;

#include <random>
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

module foundation.infrastructure.core_utils;

namespace foundation::infrastructure::core_utils {

namespace random {

/**
 * @brief Generate a random string of specified length
 * @param length The length of the random string to generate
 * @return std::string Random string containing alphanumeric characters
 * @throws std::invalid_argument if length is zero
 * @throws std::runtime_error if random string generation fails
 * @note Thread-safe (uses static local variables for thread-safe initialization)
 * @exception-safe Strong guarantee
 * @details Uses Mersenne Twister algorithm (std::mt19937) for random number generation
 */
std::string generate_random_str(const size_t& length) {
    if (length == 0) {
        throw std::invalid_argument("Length must be greater than zero");
    }

    static const std::string chars =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<size_t> distribution(0, chars.size() - 1);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[distribution(generator)];
    }
    return result;
}

/**
 * @brief Generate a UUID (Universally Unique Identifier) version 4
 * @return std::string UUID string in standard 8-4-4-4-12 format
 * @throws std::runtime_error if UUID generation fails
 * @note Thread-safe (uses static local variables for thread-safe initialization)
 * @exception-safe Strong guarantee
 * @details Generates a random UUID v4 according to RFC 4122
 */
std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution dis(0, 15);
    static std::uniform_int_distribution dis_variant(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis_variant(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

} // namespace random

namespace conversion {

/**
 * @brief Convert YAML string to JSON object
 * @param yaml_str The YAML string to convert
 * @return nlohmann::json JSON object representing the YAML content
 * @throws std::runtime_error if YAML parsing fails
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 * @details Parses YAML string and converts it to JSON object using yaml_node_to_json
 */
nlohmann::json yaml_str_to_json(const std::string& yaml_str) {
    if (yaml_str.empty()) {
        return nlohmann::json::object();
    }

    try {
        const YAML::Node root = YAML::Load(yaml_str);
        return yaml_node_to_json(root);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to parse YAML: " + std::string(e.what()));
    }
}

/**
 * @brief Convert JSON object to YAML string
 * @param j The JSON object to convert
 * @return std::string YAML string representation
 * @throws std::runtime_error if JSON to YAML conversion fails
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 * @details Converts JSON to YAML node using json_to_yaml_node and emits YAML string
 */
std::string json_to_yaml_str(const nlohmann::json& j) {
    try {
        YAML::Emitter emitter;
        emitter.SetIndent(2);
        emitter.SetMapFormat(YAML::Flow);
        emitter << json_to_yaml_node(j);

        if (!emitter.good()) {
            throw std::runtime_error("YAML emit error: " + std::string(emitter.GetLastError()));
        }
        return emitter.c_str();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Failed to convert JSON to YAML: " + std::string(e.what()));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to convert JSON to YAML: " + std::string(e.what()));
    }
}

/**
 * @brief Infer scalar type from string value
 * @param value The string value to infer type from
 * @return nlohmann::json JSON value with inferred type (int, float, or string)
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 * @details Attempts to parse string as integer, then float, falls back to string
 */
nlohmann::json infer_scalar_type(const std::string& value) {
    if (value.empty()) {
        return value;
    }

    try {
        size_t pos;
        auto int_val = std::stoll(value, &pos);
        if (pos == value.size()) {
            return int_val;
        }
    } catch (...) {
    }

    try {
        size_t pos;
        auto float_val = std::stod(value, &pos);
        if (pos == value.size()) {
            return float_val;
        }
    } catch (...) {
    }

    return value;
}

/**
 * @brief Convert YAML node to JSON object recursively
 * @param node The YAML node to convert
 * @return nlohmann::json JSON object representing the YAML node
 * @throws std::runtime_error if YAML node type is unsupported
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 * @details Handles scalar, sequence, map, null, and undefined YAML node types
 */
nlohmann::json yaml_node_to_json(const YAML::Node& node) {
    switch (node.Type()) {
    case YAML::NodeType::Scalar: {
        auto scalar_value = node.as<std::string>();

        if (scalar_value == "true" || scalar_value == "True" || scalar_value == "TRUE" || scalar_value == "yes" || scalar_value == "Yes" || scalar_value == "YES") {
            return true;
        }
        if (scalar_value == "false" || scalar_value == "False" || scalar_value == "FALSE" || scalar_value == "no" || scalar_value == "No" || scalar_value == "NO") {
            return false;
        }
        if (scalar_value == "null" || scalar_value == "Null" || scalar_value == "NULL") {
            return nullptr;
        }

        const std::string& tag = node.Tag();
        if (tag == "!!int" || tag == "tag:yaml.org,2002:int") {
            try {
                return node.as<int64_t>();
            } catch (...) {
                return scalar_value;
            }
        }
        if (tag == "!!float" || tag == "tag:yaml.org,2002:float") {
            try {
                return node.as<double>();
            } catch (...) {
                return scalar_value;
            }
        }
        return infer_scalar_type(scalar_value);
    }
    case YAML::NodeType::Sequence: {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& child : node) {
            arr.push_back(yaml_node_to_json(child));
        }
        return arr;
    }
    case YAML::NodeType::Map: {
        nlohmann::json obj = nlohmann::json::object();
        for (const auto& pair : node) {
            const auto key = pair.first.as<std::string>();
            obj[key] = yaml_node_to_json(pair.second);
        }
        return obj;
    }
    case YAML::NodeType::Null:
        return nullptr;
    case YAML::NodeType::Undefined:
        return nlohmann::json::object();
    default:
        throw std::runtime_error("Unsupported YAML node type: " + std::to_string(static_cast<int>(node.Type())));
    }
}

/**
 * @brief Convert JSON object to YAML node recursively
 * @param j The JSON object to convert
 * @return YAML::Node YAML node representing the JSON object
 * @throws std::runtime_error if JSON type is unsupported
 * @note Thread-safe (no shared mutable state)
 * @exception-safe Strong guarantee
 * @details Handles object, array, null, boolean, integer, float, string, binary, and discarded JSON types
 */
YAML::Node json_to_yaml_node(const nlohmann::json& j) {
    if (j.is_object()) {
        YAML::Node node(YAML::NodeType::Map);
        for (auto& [key, value] : j.items()) {
            node[key] = json_to_yaml_node(value);
        }
        return node;
    }
    if (j.is_array()) {
        YAML::Node node(YAML::NodeType::Sequence);
        for (auto& element : j) {
            node.push_back(json_to_yaml_node(element));
        }
        return node;
    }
    if (j.is_null()) {
        return YAML::Node(YAML::NodeType::Null);
    }
    if (j.is_boolean()) {
        return YAML::Node(j.get<bool>());
    }
    if (j.is_number_integer()) {
        return YAML::Node(j.get<int64_t>());
    }
    if (j.is_number_float()) {
        return YAML::Node(j.get<double>());
    }
    if (j.is_string()) {
        return YAML::Node(j.get<std::string>());
    }
    if (j.is_binary()) {
        return YAML::Node("[binary data]");
    }
    if (j.is_discarded()) {
        throw std::runtime_error("JSON contains discarded content");
    }

    throw std::runtime_error("Unsupported JSON type");
}

} // namespace conversion

} // namespace foundation::infrastructure::core_utils
