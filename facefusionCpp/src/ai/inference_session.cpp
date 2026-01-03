/**
 * @file inference_session.cpp
 * @brief Implementation of ONNX Runtime inference session wrapper
 * @author CodingRookie
 * @date 2024-07-12
 * @note This file contains the implementation of the InferenceSession class which provides
 *       a high-level interface for ONNX Runtime inference with support for multiple execution providers.
 */

module;
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <onnxruntime_cxx_api.h>

module inference_session;

namespace ffc::ai {

using namespace ffc::infra;

/**
 * @brief Construct a new InferenceSession object
 * @param env Optional shared pointer to ONNX Runtime environment. If null, a new one will be created.
 * @note Initializes session options with maximum graph optimization level and queries available execution providers.
 */
InferenceSession::InferenceSession(const std::shared_ptr<Ort::Env>& env) {
    m_ort_env = env;
    m_session_options = Ort::SessionOptions();
    m_session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    auto available_providers = Ort::GetAvailableProviders();
    m_available_providers.insert(available_providers.begin(), available_providers.end());
    m_logger = Logger::get_instance();
    m_memory_info = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
}

/**
 * @brief Load an ONNX model from file
 * @param model_path Path to the ONNX model file
 * @param options Configuration options for the inference session
 * @throws std::runtime_error If model path is empty, file does not exist, or session creation fails
 * @note This method is thread-safe. It resets any previously loaded model before loading the new one.
 *       The method configures execution providers based on the options and queries the model's
 *       input/output metadata after successful loading.
 */
void InferenceSession::load_model(const std::string& model_path, const Options& options) {
    if (model_path.empty()) {
        throw std::runtime_error("modelPath is empty");
    }
    if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error(std::format("modelPath: {} does not exist", model_path));
    }

    std::lock_guard lock(m_mutex);
    if (m_ort_env == nullptr) {
        m_ort_env = std::make_shared<Ort::Env>(Ort::Env(ORT_LOGGING_LEVEL_WARNING, typeid(*this).name()));
    }

    reset();
    m_options = options;
    if (m_options.execution_providers.contains(ExecutionProvider::TensorRT)) {
        append_provider_tensorrt();
    }
    if (m_options.execution_providers.contains(ExecutionProvider::CUDA)) {
        append_provider_cuda();
    }

    try {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
        auto wide_model_path = std::filesystem::path(model_path).wstring();
        m_ort_session = std::make_unique<Ort::Session>(*m_ort_env, wide_model_path.c_str(), m_session_options);
#else
        m_ort_session = std::make_unique<Ort::Session>(*m_ort_env, model_path.c_str(), m_session_options);
#endif
    } catch (const Ort::Exception& e) {
        m_logger->error(std::format("CreateSession: Ort::Exception: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (const std::exception& e) {
        m_logger->error(std::format("CreateSession: std::exception: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (...) {
        m_logger->error("CreateSession: Unknown exception occurred.");
        throw std::runtime_error("CreateSession: Unknown exception occurred.");
    }

    const size_t num_input_nodes = m_ort_session->GetInputCount();
    const size_t num_output_nodes = m_ort_session->GetOutputCount();

    m_input_names.reserve(num_input_nodes);
    m_output_names.reserve(num_output_nodes);
    m_input_names_ptrs.reserve(num_input_nodes);
    m_output_names_ptrs.reserve(num_output_nodes);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < num_input_nodes; i++) {
        m_input_names_ptrs.push_back(std::move(m_ort_session->GetInputNameAllocated(i, allocator)));
        m_input_names.push_back(m_input_names_ptrs[i].get());
        Ort::TypeInfo input_type_info = m_ort_session->GetInputTypeInfo(i);
        auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        auto input_dims = input_tensor_info.GetShape();
        m_input_node_dims.push_back(input_dims);
    }
    for (size_t i = 0; i < num_output_nodes; i++) {
        m_output_names_ptrs.push_back(std::move(m_ort_session->GetOutputNameAllocated(i, allocator)));
        m_output_names.push_back(m_output_names_ptrs[i].get());
        Ort::TypeInfo output_type_info = m_ort_session->GetOutputTypeInfo(i);
        auto output_tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
        auto output_dims = output_tensor_info.GetShape();
        m_output_node_dims.push_back(output_dims);
    }

    std::string info_msg = "Model loaded: " + model_path + ", Providers: ";
    for (const auto& provider : options.execution_providers) {
        switch (provider) {
        case ExecutionProvider::CPU:
            info_msg += "CPU ";
            break;
        case ExecutionProvider::TensorRT:
            info_msg += "TensorRT ";
            break;
        case ExecutionProvider::CUDA:
            info_msg += "CUDA ";
            break;
        default:
            break;
        }
    }

    m_logger->trace(info_msg);
    m_is_model_loaded = true;
    m_model_path = model_path;
}

/**
 * @brief Append CUDA execution provider to session options
 * @note This method checks if CUDA execution provider is available before appending.
 *       If trt_max_workspace_size is set, it configures the GPU memory limit accordingly.
 * @throws std::runtime_error If appending CUDA provider fails
 */
void InferenceSession::append_provider_cuda() {
    if (!m_available_providers.contains("CUDAExecutionProvider")) {
        m_logger->error("CUDA execution provider is not available in your environment.");
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
        m_logger->error(std::format("AppendExecutionProvider_CUDA: Ort::Exception: {}", e.what()));
        throw std::runtime_error(e.what());
    }
}

/**
 * @brief Append TensorRT execution provider to session options
 * @note This method configures TensorRT with various options including workspace size,
 *       device ID, engine caching, and embedding. It creates engine cache in ./trt_engine_cache
 *       directory if caching is enabled.
 * @throws std::runtime_error If appending TensorRT provider fails
 */
void InferenceSession::append_provider_tensorrt() {
    if (!m_available_providers.contains("TensorrtExecutionProvider")) {
        m_logger->error("TensorRT execution provider is not available in your environment.");
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

    const std::string device_id = std::to_string(m_options.execution_device_id);
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
        Ort::ThrowOnError(api.UpdateTensorRTProviderOptions(tensorrt_provider_options_v2,
                                                            keys.data(), values.data(), keys.size()));
        m_session_options.AppendExecutionProvider_TensorRT_V2(*tensorrt_provider_options_v2);
    } catch (const Ort::Exception& e) {
        m_logger->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (const std::exception& e) {
        m_logger->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (...) {
        m_logger->error("Failed to append TensorRT execution provider: Unknown error");
        throw std::runtime_error("Failed to append TensorRT execution provider: Unknown error");
    }
    api.ReleaseTensorRTProviderOptions(tensorrt_provider_options_v2);
}

/**
 * @brief Check if a model is currently loaded
 * @return true if a model is loaded, false otherwise
 */
bool InferenceSession::is_model_loaded() const {
    return m_is_model_loaded;
}

/**
 * @brief Get the path of the currently loaded model
 * @return std::string Path to the loaded model file
 */
std::string InferenceSession::get_loaded_model_path() const {
    return m_model_path;
}

/**
 * @brief Reset the session state
 * @note This method clears all session state including the loaded model, input/output metadata,
 *       and resets session options. It is called internally before loading a new model.
 */
void InferenceSession::reset() {
    m_is_model_loaded = false;
    m_model_path.clear();
    m_ort_session.reset();
    m_input_names.clear();
    m_output_names.clear();
    m_input_names_ptrs.clear();
    m_output_names_ptrs.clear();
    m_input_node_dims.clear();
    m_output_node_dims.clear();
    m_session_options = Ort::SessionOptions();
    m_session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    m_cuda_provider_options.reset();
}
} // namespace ffc::ai
