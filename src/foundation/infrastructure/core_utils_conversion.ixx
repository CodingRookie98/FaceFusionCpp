module;
#include <nlohmann/json.hpp>
#include <string>

export module foundation.infrastructure.core_utils:conversion;

namespace foundation::infrastructure::core_utils::conversion {
export nlohmann::json yaml_str_to_json(const std::string& yaml_str);
export std::string json_to_yaml_str(const nlohmann::json& j);
} // namespace foundation::infrastructure::core_utils::conversion
