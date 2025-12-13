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
namespace ffc::utils {

export nlohmann::json yamlToJson(const YAML::Node& node) {
    using json = nlohmann::json;

    if (!node || node.IsNull()) {
        return nullptr;
    }

    if (node.IsScalar()) {
        const std::string value = node.as<std::string>();

        // Optional: basic type inference
        try {
            return node.as<bool>();
        } catch (...) {}
        try {
            return node.as<int>();
        } catch (...) {}
        try {
            return node.as<double>();
        } catch (...) {}

        return value;
    }

    if (node.IsSequence()) {
        json arr = json::array();
        for (const auto& item : node) {
            arr.push_back(yamlToJson(item));
        }
        return arr;
    }

    if (node.IsMap()) {
        json obj = json::object();
        for (const auto& kv : node) {
            obj[kv.first.as<std::string>()] = yamlToJson(kv.second);
        }
        return obj;
    }

    return nullptr;
}

export std::string generateRandomString(const size_t& length) {
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
} // namespace ffc::utils