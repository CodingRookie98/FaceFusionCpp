/**
 ******************************************************************************
 * @file           : utils.ixx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 2025/12/13
 ******************************************************************************
 */
module;
#include <random>
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

export module utils;
import face;

namespace ffc::utils {

using json = nlohmann::json;

export nlohmann::json yaml_str_to_json(const std::string& yaml_str);
export nlohmann::json yaml_node_to_json(const YAML::Node& node);
export std::string json_to_yaml_str(const nlohmann::json& j);
export YAML::Node json_to_yaml_node(const nlohmann::json& j);
export YAML::Node load_yaml_file(const std::string& yaml_file_path);
export json load_json_file(const std::string& json_file_path);
export bool save_json_file(const std::string& json_file_path, const json& j, int indent = 2);
export json yaml_file_to_json(const std::string& yaml_file_path);
export std::string generate_random_str(const size_t& length);
export std::string generate_uuid();
// export std::initializer_list<Gender> get_all_genders();
// export std::initializer_list<Race> get_all_races();

/**
 * @brief 获取枚举类的所有值
 * @tparam EnumType 枚举类型
 * @return 包含枚举所有值的initializer_list
 *
 * @details
 * 这个模板函数使用C++20的模板元编程技术，通过requires约束确保只对枚举类型生效。
 * 函数会自动推断枚举的所有值并返回一个initializer_list，方便进行遍历和操作。
 *
 * @example
 * auto formats = get_all_enum_values<ImageFormat>();
 * for (auto format : formats) {
 *     // 遍历所有ImageFormat枚举值
 * }
 */
export template <typename EnumType>
    requires std::is_enum_v<EnumType>
constexpr std::initializer_list<EnumType> enum_all() {
    // 使用C++20的模板技巧自动推断枚举值
    // 这里需要为每个枚举类型提供特化实现
    static_assert(false, "请为枚举类型EnumType提供get_all_enum_values的特化实现");
    return std::initializer_list<EnumType>();
}

export template <>
constexpr std::initializer_list<Gender> enum_all<Gender>();

export template <>
constexpr std::initializer_list<Race> enum_all<Race>();
} // namespace ffc::utils