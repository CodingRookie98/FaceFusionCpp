/**
 * @file inference_session_registry.cpp
 * @brief Implementation of InferenceSessionRegistry
 */

module;
#include <algorithm>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

module foundation.ai.inference_session_registry;

import foundation.ai.inference_session;

namespace foundation::ai::inference_session {

InferenceSessionRegistry::InferenceSessionRegistry() {
    // Initialize pool with default config
    // TODO: Load config from AppConfig if available
    session_pool::PoolConfig config;
    config.max_entries = 3;
    config.idle_timeout = std::chrono::seconds(60);
    config.enable = true;
}

InferenceSessionRegistry& InferenceSessionRegistry::get_instance() {
    static InferenceSessionRegistry instance;
    return instance;
}

std::string InferenceSessionRegistry::generate_key(const std::string& model_path,
                                                   const Options& options) {
    std::stringstream ss;
    ss << model_path << "|EP:";

    // Sort providers to ensure consistent key
    std::vector<int> providers;
    for (auto ep : options.execution_providers) { providers.push_back(static_cast<int>(ep)); }
    std::sort(providers.begin(), providers.end());
    for (int p : providers) ss << p << ",";

    ss << "|Dev:" << options.execution_device_id;
    ss << "|TRT:" << options.trt_max_workspace_size << "," << options.enable_tensorrt_embed_engine
       << "," << options.enable_tensorrt_cache;

    return ss.str();
}

std::shared_ptr<InferenceSession> InferenceSessionRegistry::get_session(
    const std::string& model_path, const Options& options) {
    if (model_path.empty()) return nullptr;

    std::string key = generate_key(model_path, options);

    return m_pool.get_or_create(key, [&]() {
        auto session = std::make_shared<InferenceSession>();
        session->load_model(model_path, options);
        return session;
    });
}

void InferenceSessionRegistry::clear() {
    m_pool.clear();
}

size_t InferenceSessionRegistry::cleanup_expired() {
    return m_pool.cleanup_expired();
}

} // namespace foundation::ai::inference_session
