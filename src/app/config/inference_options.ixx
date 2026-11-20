/**
 * @file inference_options.ixx
 * @brief Translate application inference configuration into session options
 * @author CodingRookie
 * @date 2026-11-20
 * @details Bridges `config::InferenceConfig` (YAML schema) and
 *          `foundation::ai::inference_session::Options` (runtime contract), so that
 *          `inference.default_providers` actually selects execution providers instead of
 *          being silently ignored. Resolution rules are documented in
 *          docs/project/plans/provider-wiring/IMPLEMENTATION_PLAN.md (D1-D4).
 */

module;

#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

export module config.inference_options;

export import config.app;
export import foundation.ai.inference_session;

export namespace config {

namespace inference = foundation::ai::inference_session;

/**
 * @brief Parse a configured execution provider name (case-insensitive)
 * @param name Configured name, e.g. "tensorrt", "CUDA", "cpu"
 * @return Matched provider, or std::nullopt when the name is not supported
 */
std::optional<inference::ExecutionProvider> parse_execution_provider(std::string_view name) {
    std::string lowered;
    lowered.reserve(name.size());
    for (const char ch : name) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }

    if (lowered == "tensorrt") { return inference::ExecutionProvider::TensorRT; }
    if (lowered == "cuda") { return inference::ExecutionProvider::CUDA; }
    if (lowered == "cpu") { return inference::ExecutionProvider::CPU; }
    return std::nullopt;
}

/**
 * @brief Resolve configured provider priority into the provider set to register
 * @param configured Configured priority list (highest priority first)
 * @param available Providers actually available in this build/host
 * @return Providers to append to the session; never empty (CPU is the final fallback)
 * @details Takes the first configured provider that is available, then expands the
 *          runtime closure: TensorRT requires CUDA and CPU fallbacks, CUDA requires CPU.
 *          Unknown or unavailable entries are skipped; an empty or fully unresolved list
 *          falls back to `available` (auto-detected best providers).
 */
std::unordered_set<inference::ExecutionProvider> resolve_execution_providers(
    const std::vector<std::string>& configured,
    const std::unordered_set<inference::ExecutionProvider>& available) {
    if (available.empty()) { return {inference::ExecutionProvider::CPU}; }

    for (const std::string& name : configured) {
        const auto parsed = parse_execution_provider(name);
        if (!parsed.has_value() || !available.contains(*parsed)) { continue; }

        switch (*parsed) {
        case inference::ExecutionProvider::TensorRT:
            return {inference::ExecutionProvider::TensorRT, inference::ExecutionProvider::CUDA,
                    inference::ExecutionProvider::CPU};
        case inference::ExecutionProvider::CUDA:
            return {inference::ExecutionProvider::CUDA, inference::ExecutionProvider::CPU};
        case inference::ExecutionProvider::CPU: return {inference::ExecutionProvider::CPU};
        }
    }

    return available;
}

/**
 * @brief Build inference session options from the application configuration
 * @param inference_config Parsed `inference` section of app.yaml
 * @return Session options reflecting configured provider priority, device and engine cache
 */
inference::Options make_inference_options(const InferenceConfig& inference_config) {
    inference::Options options;
    options.execution_providers = resolve_execution_providers(
        inference_config.default_providers, inference::get_best_available_providers());
    options.execution_device_id = inference_config.device_id;
    options.enable_tensorrt_cache = inference_config.engine_cache.enable;
    options.engine_cache_path = inference_config.engine_cache.path;
    return options;
}

} // namespace config
