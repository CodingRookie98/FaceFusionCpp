/**
 * @file processor_param_registry.ixx
 * @brief Registry for processor parameter metadata
 * @details Enables dynamic CLI flag generation and parameter validation.
 *          Each processor registers its parameter metadata at static init time.
 */

module;

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <algorithm>

export module processor.param_registry;

export namespace domain::processor {

/**
 * @brief Supported parameter types for CLI binding
 */
enum class ParamType { String, Int, Float, Bool, Path };

/**
 * @brief Metadata for a single processor parameter
 */
struct ParamMeta {
    std::string name;                          ///< Parameter name (snake_case)
    ParamType type{ParamType::String};         ///< Value type
    std::vector<std::string> allowed_values{}; ///< Enum constraints (empty = unrestricted)
    std::string description{};                 ///< Human-readable description
    std::optional<std::pair<double, double>> range{
        std::nullopt}; ///< Numeric range (Int/Float only)
};

/**
 * @brief Metadata for a processor's complete parameter set
 */
struct ProcessorMeta {
    std::string name;              ///< Processor identifier (e.g., "face_swapper")
    std::vector<ParamMeta> params; ///< Ordered parameter list
};

/**
 * @brief Global registry for processor parameter metadata (singleton)
 */
class ProcessorParamRegistry {
public:
    static ProcessorParamRegistry& instance() {
        static ProcessorParamRegistry registry;
        return registry;
    }

    void register_processor(ProcessorMeta meta) { m_registry[meta.name] = std::move(meta); }

    [[nodiscard]] const ProcessorMeta* find(const std::string& name) const {
        auto it = m_registry.find(name);
        return it != m_registry.end() ? &it->second : nullptr;
    }

    [[nodiscard]] const std::map<std::string, ProcessorMeta>& all() const noexcept {
        return m_registry;
    }

    [[nodiscard]] std::vector<std::string> all_processor_names() const {
        std::vector<std::string> names;
        names.reserve(m_registry.size());
        for (const auto& [k, v] : m_registry) { names.push_back(k); }
        return names;
    }

    [[nodiscard]] bool is_valid_processor(const std::string& name) const {
        return m_registry.contains(name);
    }

private:
    ProcessorParamRegistry() {
        register_processor(ProcessorMeta{.name = "face_swapper",
                                         .params = {
                                             {"model",
                                              ParamType::String,
                                              {"inswapper_128", "inswapper_128_fp16"},
                                              "Swap model name"},
                                             {"face_selector_mode",
                                              ParamType::String,
                                              {"reference", "one", "many"},
                                              "Face selection mode"},
                                             {"reference_face_path",
                                              ParamType::Path,
                                              {},
                                              "Reference face image (required if mode=reference)"},
                                         }});
        register_processor(
            ProcessorMeta{.name = "face_enhancer",
                          .params = {
                              {"model",
                               ParamType::String,
                               {"codeformer", "gfpgan_1.2", "gfpgan_1.3", "gfpgan_1.4"},
                               "Enhancer model"},
                              {"blend_factor",
                               ParamType::Float,
                               {},
                               "Blend factor [0.0, 1.0]",
                               std::make_pair(0.0, 1.0)},
                              {"face_selector_mode",
                               ParamType::String,
                               {"reference", "one", "many"},
                               "Face selection mode"},
                              {"reference_face_path", ParamType::Path, {}, "Reference face image"},
                          }});
        register_processor(
            ProcessorMeta{.name = "expression_restorer",
                          .params = {
                              {"model", ParamType::String, {"live_portrait"}, "Restorer model"},
                              {"restore_factor",
                               ParamType::Float,
                               {},
                               "Restore factor [0.0, 1.0]",
                               std::make_pair(0.0, 1.0)},
                              {"face_selector_mode",
                               ParamType::String,
                               {"reference", "one", "many"},
                               "Face selection mode"},
                              {"reference_face_path", ParamType::Path, {}, "Reference face image"},
                          }});
        register_processor(ProcessorMeta{
            .name = "frame_enhancer",
            .params = {
                {"model",
                 ParamType::String,
                 {"real_esrgan_x2", "real_esrgan_x2_fp16", "real_esrgan_x4", "real_esrgan_x4_fp16",
                  "real_esrgan_x8", "real_esrgan_x8_fp16", "real_hatgan_x4"},
                 "Frame enhancer model"},
                {"enhance_factor",
                 ParamType::Float,
                 {},
                 "Enhance factor [0.0, 1.0]",
                 std::make_pair(0.0, 1.0)},
            }});
    }
    std::map<std::string, ProcessorMeta> m_registry;
};

/**
 * @brief RAII helper for static registration (mirrors ProcessorRegistrar pattern)
 */
struct ProcessorParamRegistrar {
    explicit ProcessorParamRegistrar(ProcessorMeta meta) {
        ProcessorParamRegistry::instance().register_processor(std::move(meta));
    }
};

} // namespace domain::processor
