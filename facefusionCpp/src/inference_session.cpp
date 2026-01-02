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

namespace ffc {
InferenceSession::InferenceSession(const std::shared_ptr<Ort::Env>& env) {
    ort_env_ = env;
    session_options_ = Ort::SessionOptions();
    session_options_.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    auto availableProviders = Ort::GetAvailableProviders();
    available_providers_.insert(availableProviders.begin(), availableProviders.end());
    logger_ = Logger::get_instance();
    memory_info_ = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
}

void InferenceSession::load_model(const std::string& model_path, const Options& options) {
    if (model_path.empty()) {
        throw std::runtime_error("modelPath is empty");
    }
    if (!std::filesystem::exists(model_path)) {
        throw std::runtime_error(std::format("modelPath: {} does not exist", model_path));
    }

    std::lock_guard lock(mutex_);
    if (ort_env_ == nullptr) {
        ort_env_ = std::make_shared<Ort::Env>(Ort::Env(ORT_LOGGING_LEVEL_WARNING, typeid(*this).name()));
    }

    options_ = options;
    if (options_.execution_providers.contains(ExecutionProvider::TensorRT)) {
        AppendProviderTensorrt();
    }
    if (options_.execution_providers.contains(ExecutionProvider::CUDA)) {
        AppendProviderCuda();
    }

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // windows
    const std::wstring wideModelPath(model_path.begin(), model_path.end());
    try {
        ort_session_ = std::make_unique<Ort::Session>(*ort_env_, wideModelPath.c_str(), session_options_);
    } catch (const Ort::Exception& e) {
        logger_->error(std::format("CreateSession: Ort::Exception: {}", e.what()));
        return;
    } catch (const std::exception& e) {
        logger_->error(std::format("CreateSession: std::exception: {}", e.what()));
        return;
    } catch (...) {
        logger_->error("CreateSession: Unknown exception occurred.");
        return;
    }
#else
    // linux
    ort_session_ = std::make_unique<Ort::Session>(Ort::Session(*m_env, modelPath.c_str(), session_options_));
#endif

    const size_t numInputNodes = ort_session_->GetInputCount();
    const size_t numOutputNodes = ort_session_->GetOutputCount();

    input_names_.reserve(numInputNodes);
    output_names_.reserve(numOutputNodes);
    input_names_ptrs_.reserve(numInputNodes);
    output_names_ptrs_.reserve(numOutputNodes);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < numInputNodes; i++) {
        input_names_ptrs_.push_back(std::move(ort_session_->GetInputNameAllocated(i, allocator)));
        input_names_.push_back(input_names_ptrs_[i].get());
        Ort::TypeInfo inputTypeInfo = ort_session_->GetInputTypeInfo(i);
        auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
        auto inputDims = inputTensorInfo.GetShape();
        input_node_dims_.push_back(inputDims);
    }
    for (size_t i = 0; i < numOutputNodes; i++) {
        output_names_ptrs_.push_back(std::move(ort_session_->GetOutputNameAllocated(i, allocator)));
        output_names_.push_back(output_names_ptrs_[i].get());
        Ort::TypeInfo outputTypeInfo = ort_session_->GetOutputTypeInfo(i);
        auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
        auto outputDims = outputTensorInfo.GetShape();
        output_node_dims_.push_back(outputDims);
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

    logger_->trace(infoMsg);
    is_model_loaded_ = true;
    model_path_ = model_path;
}

void InferenceSession::AppendProviderCuda() {
    if (!available_providers_.contains("CUDAExecutionProvider")) {
        logger_->error("CUDA execution provider is not available in your environment.");
        return;
    }
    cuda_provider_options_ = std::make_unique<OrtCUDAProviderOptions>();
    cuda_provider_options_->device_id = options_.execution_device_id;
    if (options_.trt_max_workspace_size > 0) {
        cuda_provider_options_->gpu_mem_limit = options_.trt_max_workspace_size * (1 << 30);
    }
    try {
        session_options_.AppendExecutionProvider_CUDA(*cuda_provider_options_);
    } catch (const Ort::Exception& e) {
        logger_->error(std::format("AppendExecutionProvider_CUDA: Ort::Exception: {}", e.what()));
        throw std::runtime_error(e.what());
    }
}

void InferenceSession::AppendProviderTensorrt() {
    if (!available_providers_.contains("TensorrtExecutionProvider")) {
        logger_->error("TensorRT execution provider is not available in your environment.");
        return;
    }
    std::vector<const char*> keys;
    std::vector<const char*> values;
    const auto& api = Ort::GetApi();
    OrtTensorRTProviderOptionsV2* tensorrtProviderOptionsV2;
    api.CreateTensorRTProviderOptions(&tensorrtProviderOptionsV2);

    std::string trtMaxWorkSpaceSiz;
    if (options_.trt_max_workspace_size > 0) {
        trtMaxWorkSpaceSiz = std::to_string(options_.trt_max_workspace_size);
        keys.emplace_back("trt_max_workspace_size");
        values.emplace_back(trtMaxWorkSpaceSiz.c_str());
    }

    const std::string deviceId = std::to_string(options_.execution_device_id);
    keys.emplace_back("device_id");
    values.emplace_back(deviceId.c_str());

    std::string enableTensorrtCache;
    std::string enableTensorrtEmbedEngine;
    std::string tensorrtEmbedEnginePath;
    if (options_.enable_tensorrt_embed_engine) {
        enableTensorrtCache = std::to_string(options_.enable_tensorrt_cache);
        enableTensorrtEmbedEngine = std::to_string(options_.enable_tensorrt_embed_engine);
        tensorrtEmbedEnginePath = "./trt_engine_cache";

        keys.emplace_back("trt_engine_cache_enable");
        values.emplace_back(enableTensorrtCache.c_str());
        keys.emplace_back("trt_dump_ep_context_model");
        values.emplace_back(enableTensorrtEmbedEngine.c_str());
        keys.emplace_back("trt_ep_context_file_path");
        values.emplace_back(tensorrtEmbedEnginePath.c_str());
    }

    std::string tensorrtCachePath;
    if (options_.enable_tensorrt_cache) {
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
        session_options_.AppendExecutionProvider_TensorRT_V2(*tensorrtProviderOptionsV2);
    } catch (const Ort::Exception& e) {
        logger_->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (const std::exception& e) {
        logger_->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
        throw std::runtime_error(e.what());
    } catch (...) {
        logger_->error("Failed to append TensorRT execution provider: Unknown error");
        throw std::runtime_error("Failed to append TensorRT execution provider: Unknown error");
    }
    api.ReleaseTensorRTProviderOptions(tensorrtProviderOptionsV2);
}

bool InferenceSession::IsModelLoaded() const {
    return is_model_loaded_;
}

std::string InferenceSession::GetModelPath() const {
    return model_path_;
}
} // namespace ffc
