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
#include <optional>
#include <format>
#include <filesystem>
#include <system_error>

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

std::shared_ptr<InferenceSessionRegistry> InferenceSessionRegistry::get_instance() {
    static std::once_flag flag;
    static std::shared_ptr<InferenceSessionRegistry> instance;
    std::call_once(flag, [&]() { instance = std::make_shared<InferenceSessionRegistry>(); });
    return instance;
}

namespace {
// 模型文件指纹：size + last_write_time；stat 失败返回 nullopt（省略指纹段）
std::optional<std::string> file_fingerprint(const std::string& model_path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(model_path, ec);
    if (ec) return std::nullopt;
    const auto mtime = std::filesystem::last_write_time(model_path, ec);
    if (ec) return std::nullopt;
    return std::format("{}:{}", size, mtime.time_since_epoch().count());
}
} // namespace

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

    // 文件指纹：模型文件更新（同路径覆盖）后 key 变化 → 热更新生效
    if (auto fp = file_fingerprint(model_path)) { ss << "|File:" << *fp; }

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

void InferenceSessionRegistry::preload_session(const std::string& model_path,
                                               const Options& options,
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
