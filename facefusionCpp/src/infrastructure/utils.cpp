/**
 ******************************************************************************
 * @file           : utils.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/25
 ******************************************************************************
 */
module;
#include <random>
#include <fstream>
#include <regex>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

module utils;
import face;

using json = nlohmann::json;

namespace ffc::infra::utils {

json yaml_str_to_json(const std::string& yaml_str) {
    if (yaml_str.empty()) {
        return json::object();
    }

    try {
        const YAML::Node root = YAML::Load(yaml_str);
        return yaml_node_to_json(root);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(std::string("YAML parse error: ") + e.what());
    }
}

// 辅助函数：智能推断标量类型
json infer_scalar_type(const std::string& value) {
    // 检查是否为整数
    if (std::regex_match(value, std::regex(R"(^-?\d+$)"))) {
        try {
            return std::stoll(value);
        } catch (...) {
            // 如果转换失败，回退到字符串
        }
    }

    // 检查是否为浮点数（包含小数点和科学计数法）
    if (std::regex_match(value, std::regex(R"(^-?\d+(\.\d+)?([eE][+-]?\d+)?$)"))) {
        try {
            return std::stod(value);
        } catch (...) {
            // 如果转换失败，回退到字符串
        }
    }

    // 默认返回字符串
    return value;
}

nlohmann::json yaml_node_to_json(const YAML::Node& node) {
    switch (node.Type()) {
    case YAML::NodeType::Scalar: {
        // 优化类型推断逻辑
        auto scalar_value = node.as<std::string>();

        // 布尔值检查（一次性完成）
        if (scalar_value == "true" || scalar_value == "True" || scalar_value == "TRUE" || scalar_value == "yes" || scalar_value == "Yes" || scalar_value == "YES") {
            return true;
        }
        if (scalar_value == "false" || scalar_value == "False" || scalar_value == "FALSE" || scalar_value == "no" || scalar_value == "No" || scalar_value == "NO") {
            return false;
        }

        // null值检查
        if (scalar_value == "null" || scalar_value == "Null" || scalar_value == "NULL") {
            return nullptr;
        }

        // 使用YAML::Node的Tag信息进行类型推断
        const std::string& tag = node.Tag();
        if (tag == "!!int" || tag == "tag:yaml.org,2002:int") {
            try {
                return node.as<int64_t>();
            } catch (...) {
                // 如果标签是int但转换失败，回退到字符串
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
        // 如果没有明确标签，使用更智能的推断
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
        return {}; // 返回空JSON而不是抛出异常
    default:
        throw std::runtime_error("Unsupported YAML node type: " + std::to_string(static_cast<int>(node.Type())));
    }
}

std::string json_to_yaml_str(const nlohmann::json& j) {
    try {
        YAML::Emitter emitter;
        emitter.SetIndent(2);                      // 设置缩进
        emitter.SetMapFormat(YAML::Flow);          // 设置流式格式
        emitter.Write(Dump(json_to_yaml_node(j))); // 修复递归调用错误

        if (!emitter.good()) {
            throw std::runtime_error("YAML emit error: " + std::string(emitter.GetLastError()));
        }
        return emitter.c_str();
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(std::string("YAML emit error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("YAML conversion error: ") + e.what());
    }
}

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
        // 处理二进制数据
        return YAML::Node("[binary data]");
    }
    if (j.is_discarded()) {
        throw std::runtime_error("JSON contains discarded content");
    }

    throw std::runtime_error("Unsupported JSON type");
}

YAML::Node load_yaml_file(const std::string& yaml_file_path) {
    try {
        return YAML::LoadFile(yaml_file_path);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(std::string("YAML load error: ") + e.what());
    }
}

json yaml_file_to_json(const std::string& yaml_file_path) {
    try {
        const YAML::Node root = load_yaml_file(yaml_file_path);
        return yaml_node_to_json(root);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(std::string("YAML to JSON error: ") + e.what());
    }
}

std::string generate_random_str(const size_t& length) {
    if (length == 0) {
        return "";
    }
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

std::string generate_uuid() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution dis(0, 15);
    static std::uniform_int_distribution dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    return ss.str();
}

json load_json_file(const std::string& json_file_path) {
    try {
        std::ifstream in(json_file_path);
        json j;
        in >> j;
        return j;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON load error: ") + e.what());
    }
}

bool save_json_file(const std::string& json_file_path, const json& j, int indent) {
    indent = std::max(0, indent);
    try {
        std::ofstream out(json_file_path);
        out << j.dump(indent);
        return true;
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON save error: ") + e.what());
    }
}

std::initializer_list<Gender> get_all_genders() {
    return {Gender::Male, Gender::Female};
}

std::initializer_list<Race> get_all_races() {
    return {Race::Black, Race::Latino, Race::Indian, Race::Asian, Race::Arabic, Race::White};
}

template <>
constexpr std::initializer_list<Gender> enum_all<Gender>() {
    return {Gender::Male, Gender::Female};
}

template <>
constexpr std::initializer_list<Race> enum_all<Race>() {
    return {Race::Black, Race::Latino, Race::Indian, Race::Asian, Race::Arabic, Race::White};
}
} // namespace ffc::infra::utils