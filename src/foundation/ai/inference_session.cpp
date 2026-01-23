/**
 * @file inference_session.cpp
 * @brief Implementation of ONNX Runtime inference session wrapper
 */

module;
#include <algorithm>
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <onnxruntime_cxx_api.h>
#include <format>
#include <memory>
#include <string>
#include <vector>
#include <cctype>
#include <sstream>

module foundation.ai.inference_session;
import foundation.infrastructure.logger;

namespace foundation::ai::inference_session {

using namespace foundation::infrastructure;

/**
 * @brief Static ONNX Runtime Environment to ensure it outlives all sessions.
 * Using a leaked pointer to avoid destruction order issues during process exit,
 * which is a known cause of SEH exceptions with TensorRT/CUDA providers.
 */
static const Ort::Env& get_static_env() {
    static auto* const kEnv = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "FaceFusionCpp");
    return *kEnv;
}

// Get available providers from ONNX Runtime and return best ones
std::unordered_set<ExecutionProvider> get_best_available_providers() {
    // Check for environment variable to force CPU (useful for CI or troubleshooting)
    // Using a simple check to avoid discovery-time crashes seen previously
    if (const char* env_p = std::getenv("FACEFUSION_PROVIDER")) {
        const std::string kProvider = env_p;
        if (kProvider == "cpu" || kProvider == "CPU") { return {ExecutionProvider::CPU}; }
    }

    std::unordered_set<ExecutionProvider> result;
    auto available = Ort::GetAvailableProviders();
    std::unordered_set<std::string> available_set(available.begin(), available.end());

    // Priority: TensorRT > CUDA > CPU
    if (available_set.contains("TensorrtExecutionProvider")) {
        result.insert(ExecutionProvider::TensorRT);
        result.insert(ExecutionProvider::CUDA); // TensorRT needs CUDA fallback
        result.insert(ExecutionProvider::CPU);
    } else if (available_set.contains("CUDAExecutionProvider")) {
        result.insert(ExecutionProvider::CUDA);
        result.insert(ExecutionProvider::CPU);
    } else {
        result.insert(ExecutionProvider::CPU);
    }

    return result;
}

struct InferenceSession::Impl {
    std::unique_ptr<Ort::Session> m_ort_session;
    Ort::SessionOptions m_session_options;
    Ort::RunOptions m_run_options;
    std::unique_ptr<OrtCUDAProviderOptions> m_cuda_provider_options{nullptr};
    std::vector<const char*> m_input_names;
    std::vector<const char*> m_output_names;
    std::vector<std::vector<int64_t>> m_input_node_dims;
    std::vector<std::vector<int64_t>> m_output_node_dims;
    std::unique_ptr<Ort::MemoryInfo> m_memory_info;

    std::recursive_mutex m_mutex;
    // m_ort_env removed: using static global env via get_static_env()
    std::unordered_set<std::string> m_available_providers;
    Options m_options;
    std::vector<Ort::AllocatedStringPtr> m_input_names_ptrs;
    std::vector<Ort::AllocatedStringPtr> m_output_names_ptrs;
    std::shared_ptr<logger::Logger> m_logger;
    bool m_is_model_loaded = false;
    std::string m_model_path;

    Impl() {
        m_session_options = Ort::SessionOptions();
        m_session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);

        auto available_providers = Ort::GetAvailableProviders();
        m_available_providers.insert(available_providers.begin(), available_providers.end());

        m_memory_info = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
    }

    ~Impl() {
        // Explicit destruction order to avoid TensorRT SEH exceptions
        m_input_names.clear();
        m_output_names.clear();
        m_input_names_ptrs.clear();
        m_output_names_ptrs.clear();
        m_input_node_dims.clear();
        m_output_node_dims.clear();
        m_ort_session.reset();
        m_cuda_provider_options.reset();
        m_memory_info.reset();
    }

    void ensure_resources() {
        if (!m_logger) { m_logger = logger::Logger::get_instance(); }
        // Env is now static, no need to ensure specific instance
    }

    void reset() {
        std::lock_guard lock(m_mutex);
        reset_internal();
    }

    void reset_internal() {
        m_is_model_loaded = false;
        m_model_path.clear();
        m_input_names.clear();
        m_output_names.clear();
        m_input_names_ptrs.clear();
        m_output_names_ptrs.clear();
        m_input_node_dims.clear();
        m_output_node_dims.clear();
        m_ort_session.reset();
        m_cuda_provider_options.reset();
        m_session_options = Ort::SessionOptions();
        m_session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    }

    void append_provider_cuda() {
        if (!m_available_providers.contains("CUDAExecutionProvider")) {
            m_logger->warn("CUDA execution provider is not available in your environment.");
            return;
        }
        m_cuda_provider_options = std::make_unique<OrtCUDAProviderOptions>();
        m_cuda_provider_options->device_id = m_options.execution_device_id;
        if (m_options.trt_max_workspace_size > 0) {
            m_cuda_provider_options->gpu_mem_limit = m_options.trt_max_workspace_size * (1 << 30);
        }
        try {
            m_session_options.AppendExecutionProvider_CUDA(*m_cuda_provider_options);
        } catch (const Ort::Exception& e) {
            m_logger->error(
                std::format("AppendExecutionProvider_CUDA: Ort::Exception: {}", e.what()));
            throw std::runtime_error(e.what());
        }
    }

    void append_provider_tensorrt() {
        if (!m_available_providers.contains("TensorrtExecutionProvider")) {
            m_logger->warn("TensorRT execution provider is not available in your environment.");
            return;
        }

        std::vector<const char*> keys;
        std::vector<const char*> values;
        const auto& api = Ort::GetApi();
        OrtTensorRTProviderOptionsV2* tensorrt_provider_options_v2;
        api.CreateTensorRTProviderOptions(&tensorrt_provider_options_v2);

        std::string trt_max_workspace_size_str;
        if (m_options.trt_max_workspace_size > 0) {
            trt_max_workspace_size_str = std::to_string(m_options.trt_max_workspace_size);
            keys.emplace_back("trt_max_workspace_size");
            values.emplace_back(trt_max_workspace_size_str.c_str());
        }

        std::string device_id = std::to_string(m_options.execution_device_id);
        keys.emplace_back("device_id");
        values.emplace_back(device_id.c_str());

        std::string enable_tensorrt_cache;
        std::string enable_tensorrt_embed_engine;
        std::string tensorrt_embed_engine_path;
        if (m_options.enable_tensorrt_embed_engine) {
            enable_tensorrt_cache = std::to_string(m_options.enable_tensorrt_cache);
            enable_tensorrt_embed_engine = std::to_string(m_options.enable_tensorrt_embed_engine);
            tensorrt_embed_engine_path = "./trt_engine_cache";

            keys.emplace_back("trt_engine_cache_enable");
            values.emplace_back(enable_tensorrt_cache.c_str());
            keys.emplace_back("trt_dump_ep_context_model");
            values.emplace_back(enable_tensorrt_embed_engine.c_str());
            keys.emplace_back("trt_ep_context_file_path");
            values.emplace_back(tensorrt_embed_engine_path.c_str());
        }

        std::string tensorrt_cache_path;
        if (m_options.enable_tensorrt_cache) {
            if (enable_tensorrt_embed_engine.empty()) {
                keys.emplace_back("trt_engine_cache_enable");
                values.emplace_back("1");
                tensorrt_cache_path = "./trt_engine_cache/trt_engines";
            } else {
                tensorrt_cache_path = "trt_engines";
            }

            keys.emplace_back("trt_engine_cache_path");
            values.emplace_back(tensorrt_cache_path.c_str());
        }

        try {
            Ort::ThrowOnError(api.UpdateTensorRTProviderOptions(
                tensorrt_provider_options_v2, keys.data(), values.data(), keys.size()));
            m_session_options.AppendExecutionProvider_TensorRT_V2(*tensorrt_provider_options_v2);
        } catch (const std::exception& e) {
            m_logger->warn(
                std::format("Failed to append TensorRT execution provider: {}", e.what()));
        }
        api.ReleaseTensorRTProviderOptions(tensorrt_provider_options_v2);
    }

    void load_model(const std::string& model_path, const Options& options) {
        if (model_path.empty()) { throw std::runtime_error("modelPath is empty"); }
        if (!std::filesystem::exists(model_path)) {
            throw std::runtime_error(std::format("modelPath: {} does not exist", model_path));
        }

        std::lock_guard lock(m_mutex);
        ensure_resources();

        if (m_is_model_loaded && m_model_path == model_path && m_options == options) {
            m_logger->trace(
                std::format("Model already loaded with same options, skipping: {}", model_path));
            return;
        }

        reset_internal();
        m_options = options;

        auto providers_to_use = m_options.execution_providers;
        if (providers_to_use.empty()) {
            providers_to_use = get_best_available_providers();
            m_logger->info(
                "Auto-detected execution providers: "
                + std::string(
                    providers_to_use.contains(ExecutionProvider::TensorRT) ? "TensorRT, " : "")
                + std::string(providers_to_use.contains(ExecutionProvider::CUDA) ? "CUDA, " : "")
                + std::string(providers_to_use.contains(ExecutionProvider::CPU) ? "CPU" : ""));
        }

        if (providers_to_use.contains(ExecutionProvider::TensorRT)) { append_provider_tensorrt(); }
        if (providers_to_use.contains(ExecutionProvider::CUDA)) { append_provider_cuda(); }

        try {
#if defined(WIN32) || defined(_WIN32)
            auto wide_model_path = std::filesystem::path(model_path).wstring();
            m_ort_session = std::make_unique<Ort::Session>(
                get_static_env(), wide_model_path.c_str(), m_session_options);
#else
            m_ort_session = std::make_unique<Ort::Session>(get_static_env(), model_path.c_str(),
                                                           m_session_options);
#endif
            std::string providers_str;
            if (providers_to_use.contains(ExecutionProvider::TensorRT))
                providers_str += "TensorRT, ";
            if (providers_to_use.contains(ExecutionProvider::CUDA)) providers_str += "CUDA, ";
            providers_str += "CPU"; // Always available/fallback

            m_logger->info(std::format("InferenceSession created for model: {} | Providers: {}",
                                       model_path, providers_str));
        } catch (const std::exception& e) {
            m_logger->error(std::format("CreateSession: {}", e.what()));
            throw;
        }

        const size_t num_input_nodes = m_ort_session->GetInputCount();
        const size_t num_output_nodes = m_ort_session->GetOutputCount();

        m_input_names.reserve(num_input_nodes);
        m_input_names_ptrs.reserve(num_input_nodes);
        m_output_names.reserve(num_output_nodes);
        m_output_names_ptrs.reserve(num_output_nodes);
        m_input_node_dims.reserve(num_input_nodes);
        m_output_node_dims.reserve(num_output_nodes);

        Ort::AllocatorWithDefaultOptions allocator;

        for (size_t i = 0; i < num_input_nodes; i++) {
            m_input_names_ptrs.push_back(
                std::move(m_ort_session->GetInputNameAllocated(i, allocator)));
            m_input_names.push_back(m_input_names_ptrs[i].get());

            auto type_info = m_ort_session->GetInputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            m_input_node_dims.push_back(tensor_info.GetShape());
        }

        for (size_t i = 0; i < num_output_nodes; i++) {
            m_output_names_ptrs.push_back(
                std::move(m_ort_session->GetOutputNameAllocated(i, allocator)));
            m_output_names.push_back(m_output_names_ptrs[i].get());

            auto type_info = m_ort_session->GetOutputTypeInfo(i);
            auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
            m_output_node_dims.push_back(tensor_info.GetShape());
        }

        m_is_model_loaded = true;
        m_model_path = model_path;
        m_logger->trace("Model loaded: " + model_path);
    }
};

InferenceSession::InferenceSession() : m_impl(std::make_unique<Impl>()) {}
InferenceSession::~InferenceSession() = default;

void InferenceSession::load_model(const std::string& model_path, const Options& options) {
    m_impl->load_model(model_path, options);
}

bool InferenceSession::is_model_loaded() const {
    return m_impl->m_is_model_loaded;
}

std::string InferenceSession::get_loaded_model_path() const {
    return m_impl->m_model_path;
}

std::vector<Ort::Value> InferenceSession::run(const std::vector<Ort::Value>& input_tensors) {
    if (!m_impl->m_is_model_loaded) { throw std::runtime_error("Model not loaded"); }
    return m_impl->m_ort_session->Run(m_impl->m_run_options, m_impl->m_input_names.data(),
                                      input_tensors.data(), input_tensors.size(),
                                      m_impl->m_output_names.data(), m_impl->m_output_names.size());
}

std::vector<std::vector<int64_t>> InferenceSession::get_input_node_dims() const {
    return m_impl->m_input_node_dims;
}

std::vector<std::vector<int64_t>> InferenceSession::get_output_node_dims() const {
    return m_impl->m_output_node_dims;
}

std::vector<std::string> InferenceSession::get_input_names() const {
    std::vector<std::string> names;
    names.reserve(m_impl->m_input_names.size());
    for (const char* name : m_impl->m_input_names) { names.emplace_back(name); }
    return names;
}

std::vector<std::string> InferenceSession::get_output_names() const {
    std::vector<std::string> names;
    names.reserve(m_impl->m_output_names.size());
    for (const char* name : m_impl->m_output_names) { names.emplace_back(name); }
    return names;
}

} // namespace foundation::ai::inference_session
