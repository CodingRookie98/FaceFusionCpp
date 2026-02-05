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
    // Default initialization
}

void InferenceSessionRegistry::configure(const session_pool::PoolConfig& config,
                                         const std::string& cache_path) {
    m_pool.set_config(config);
    m_cache_path = cache_path;
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
        auto session_opts = options;
        if (session_opts.engine_cache_path.empty()) {
            session_opts.engine_cache_path = m_cache_path;
        }
        session->load_model(model_path, session_opts);
        return session;
    });
}

void InferenceSessionRegistry::preload_session(const std::string& model_path, const Options& options,
                                               std::shared_ptr<InferenceSession> session) {
    std::string key = generate_key(model_path, options);
    m_pool.evict(key);
    m_pool.get_or_create(key, [session]() { return session; });
}

void InferenceSessionRegistry::clear() {
    m_pool.clear();
}

size_t InferenceSessionRegistry::cleanup_expired() {
    return m_pool.cleanup_expired();
}

} // namespace foundation::ai::inference_session
