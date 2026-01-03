/**
 ******************************************************************************
 * @file           : ort_session.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-12
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <filesystem>
#include <mutex>
#include <onnxruntime_cxx_api.h>

module inference_session;

namespace ffc::ai {

using namespace ffc::infra;

InferenceSession::InferenceSession(const std::shared_ptr<Ort::Env>& env) {
    m_ort_env = env;
    m_session_options = Ort::SessionOptions();
    m_session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    auto availableProviders = Ort::GetAvailableProviders();
    m_available_providers.insert(availableProviders.begin(), availableProviders.end());
    m_logger = Logger::get_instance();
    m_memory_info = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
}

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

    m_options = options;
    if (m_options.execution_providers.contains(ExecutionProvider::TensorRT)) {
        append_provider_tensorrt();
    }
    if (m_options.execution_providers.contains(ExecutionProvider::CUDA)) {
        append_provider_cuda();
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // windows
    const std::wstring wideModelPath(model_path.begin(), model_path.end());
    try {
        m_ort_session = std::make_unique<Ort::Session>(*m_ort_env, wideModelPath.c_str(), m_session_options);
    } catch (const Ort::Exception& e) {
        m_logger->error(std::format("CreateSession: Ort::Exception: {}", e.what()));
        return;
    } catch (const std::exception& e) {
        m_logger->error(std::format("CreateSession: std::exception: {}", e.what()));
        return;
    } catch (...) {
        m_logger->error("CreateSession: Unknown exception occurred.");
        return;
    }
#else
    // linux
    m_ort_session = std::make_unique<Ort::Session>(Ort::Session(*m_env, modelPath.c_str(), session_options_));
#endif

    const size_t numInputNodes = m_ort_session->GetInputCount();
    const size_t numOutputNodes = m_ort_session->GetOutputCount();

    m_input_names.reserve(numInputNodes);
    m_output_names.reserve(numOutputNodes);
    m_input_names_ptrs.reserve(numInputNodes);
    m_output_names_ptrs.reserve(numOutputNodes);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < numInputNodes; i++) {
        m_input_names_ptrs.push_back(std::move(m_ort_session->GetInputNameAllocated(i, allocator)));
        m_input_names.push_back(m_input_names_ptrs[i].get());
        Ort::TypeInfo inputTypeInfo = m_ort_session->GetInputTypeInfo(i);
        auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
        auto inputDims = inputTensorInfo.GetShape();
        m_input_node_dims.push_back(inputDims);
    }
    for (size_t i = 0; i < numOutputNodes; i++) {
        m_output_names_ptrs.push_back(std::move(m_ort_session->GetOutputNameAllocated(i, allocator)));
        m_output_names.push_back(m_output_names_ptrs[i].get());
        Ort::TypeInfo outputTypeInfo = m_ort_session->GetOutputTypeInfo(i);
        auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
        auto outputDims = outputTensorInfo.GetShape();
        m_output_node_dims.push_back(outputDims);
    }

    std::string infoMsg = "Model loaded: " + model_path + ", Providers: ";
    for (const auto& provider : options.execution_providers) {
        switch (provider) {
        case ExecutionProvider::CPU:
            infoMsg += "CPU ";
            break;
        case ExecutionProvider::TensorRT:
            infoMsg += "TensorRT ";
            break;
        case ExecutionProvider::CUDA:
            infoMsg += "CUDA ";
            break;
        default:
            break;
        }
    }

    m_logger->trace(infoMsg);
    m_is_model_loaded = true;
    m_model_path = model_path;
}

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

void InferenceSession::append_provider_tensorrt() {
    if (!m_available_providers.contains("TensorrtExecutionProvider")) {
        m_logger->error("TensorRT execution provider is not available in your environment.");
        return;
    }
    std::vector<const char*> keys;
    std::vector<const char*> values;
    const auto& api = Ort::GetApi();
    OrtTensorRTProviderOptionsV2* tensorrtProviderOptionsV2;
    api.CreateTensorRTProviderOptions(&tensorrtProviderOptionsV2);

    std::string trtMaxWorkSpaceSiz;
    if (m_options.trt_max_workspace_size > 0) {
        trtMaxWorkSpaceSiz = std::to_string(m_options.trt_max_workspace_size);
        keys.emplace_back("trt_max_workspace_size");
        values.emplace_back(trtMaxWorkSpaceSiz.c_str());
    }

    const std::string deviceId = std::to_string(m_options.execution_device_id);
    keys.emplace_back("device_id");
    values.emplace_back(deviceId.c_str());

    std::string enableTensorrtCache;
    std::string enableTensorrtEmbedEngine;
    std::string tensorrtEmbedEnginePath;
    if (m_options.enable_tensorrt_embed_engine) {
        enableTensorrtCache = std::to_string(m_options.enable_tensorrt_cache);
        enableTensorrtEmbedEngine = std::to_string(m_options.enable_tensorrt_embed_engine);
        tensorrtEmbedEnginePath = "./trt_engine_cache";

        keys.emplace_back("trt_engine_cache_enable");
        values.emplace_back(enableTensorrtCache.c_str());
        keys.emplace_back("trt_dump_ep_context_model");
        values.emplace_back(enableTensorrtEmbedEngine.c_str());
        keys.emplace_back("trt_ep_context_file_path");
        values.emplace_back(tensorrtEmbedEnginePath.c_str());
    }

    std::string tensorrtCachePath;
    if (m_options.enable_tensorrt_cache) {
        if (enableTensorrtEmbedEngine.empty()) {
            keys.emplace_back("trt_engine_cache_enable");
            values.emplace_back("1");
            tensorrtCachePath = "./trt_engine_cache/trt_engines";
        } else {
            tensorrtCachePath = "trt_engines";
        }

        keys.emplace_back("trt_engine_cache_path");
        values.emplace_back(tensorrtCachePath.c_str());
    }

    try {
        Ort::ThrowOnError(api.UpdateTensorRTProviderOptions(tensorrtProviderOptionsV2,
                                                            keys.data(), values.data(), keys.size()));
        m_session_options.AppendExecutionProvider_TensorRT_V2(*tensorrtProviderOptionsV2);
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
    api.ReleaseTensorRTProviderOptions(tensorrtProviderOptionsV2);
}

bool InferenceSession::is_model_loaded() const {
    return m_is_model_loaded;
}

std::string InferenceSession::get_loaded_model_path() const {
    return m_model_path;
}
} // namespace ffc::ai
