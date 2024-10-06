/**
 ******************************************************************************
 * @file           : ort_session.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-12
 ******************************************************************************
 */

#include "inference_session.h"
#include <iostream>
#include "config.h"

namespace Ffc {
InferenceSession::InferenceSession(const std::shared_ptr<Ort::Env> &env) {
    m_env = env;
    m_config = Ffc::Config::getInstance();
    m_sessionOptions = Ort::SessionOptions();
    m_sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
    auto availableProviders = Ort::GetAvailableProviders();
    m_availableProviders.insert(availableProviders.begin(), availableProviders.end());

    if (m_config->m_executionProviders.contains(TensorRT)) {
        appendProviderTensorrt();
    }
    if (m_config->m_executionProviders.contains(CUDA)) {
        appendProviderCUDA();
    }
}

void InferenceSession::createSession(const std::string &modelPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    // windows
    std::wstring wideModelPath(modelPath.begin(), modelPath.end());
    try {
        m_ortSession = std::make_unique<Ort::Session>(*m_env, wideModelPath.c_str(), m_sessionOptions);
    } catch (const Ort::Exception &e) {
        m_logger->error(std::format("CreateSession: Ort::Exception: {}", e.what()));
        return;
    } catch (const std::exception &e) {
        m_logger->error(std::format("CreateSession: std::exception: {}", e.what()));
        return;
    } catch (...) {
        m_logger->error("CreateSession: Unknown exception occurred.");
        return;
    }
#else
    // linux
    m_ortSession = std::make_unique<Ort::Session>(Ort::Session(*m_env, modelPath.c_str(), m_sessionOptions));
#endif

    size_t numInputNodes = m_ortSession->GetInputCount();
    size_t numOutputNodes = m_ortSession->GetOutputCount();

    m_inputNames.reserve(numInputNodes);
    m_outputNames.reserve(numOutputNodes);
    m_inputNamesPtrs.reserve(numInputNodes);
    m_outputNamesPtrs.reserve(numOutputNodes);

    Ort::AllocatorWithDefaultOptions allocator;
    for (size_t i = 0; i < numInputNodes; i++) {
        m_inputNamesPtrs.push_back(std::move(m_ortSession->GetInputNameAllocated(i, allocator)));
        m_inputNames.push_back(m_inputNamesPtrs[i].get());
        Ort::TypeInfo inputTypeInfo = m_ortSession->GetInputTypeInfo(i);
        auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
        auto inputDims = inputTensorInfo.GetShape();
        m_inputNodeDims.push_back(inputDims);
    }
    for (size_t i = 0; i < numOutputNodes; i++) {
        m_outputNamesPtrs.push_back(std::move(m_ortSession->GetOutputNameAllocated(i, allocator)));
        m_outputNames.push_back(m_outputNamesPtrs[i].get());
        Ort::TypeInfo outputTypeInfo = m_ortSession->GetOutputTypeInfo(i);
        auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
        auto outputDims = outputTensorInfo.GetShape();
        m_outputNodeDims.push_back(outputDims);
    }
}

void InferenceSession::appendProviderCUDA() {
    if (!m_availableProviders.contains("CUDAExecutionProvider")) {
        m_logger->error("CUDA execution provider is not available in your environment.");
        return;
    }
    m_cudaProviderOptions = std::make_shared<OrtCUDAProviderOptions>();
    m_cudaProviderOptions->device_id = m_config->m_executionDeviceId;
    if (m_config->m_trtMaxWorkspaceSize > 0) {
        m_cudaProviderOptions->gpu_mem_limit = m_config->m_trtMaxWorkspaceSize * (1 << 30);
    }
    m_sessionOptions.AppendExecutionProvider_CUDA(*m_cudaProviderOptions);
}

void InferenceSession::appendProviderTensorrt() {
    if (!m_availableProviders.contains("TensorrtExecutionProvider")) {
        m_logger->error("TensorRT execution provider is not available in your environment.");
        return;
    }
    std::vector<const char *> keys;
    std::vector<const char *> values;
    const auto &api = Ort::GetApi();
    OrtTensorRTProviderOptionsV2 *tensorrtProviderOptionsV2;
    api.CreateTensorRTProviderOptions(&tensorrtProviderOptionsV2);

    std::string trtMaxWorkSpaceSiz;
    if (m_config->m_trtMaxWorkspaceSize > 0) {
        trtMaxWorkSpaceSiz = std::to_string(m_config->m_trtMaxWorkspaceSize);
        keys.emplace_back("trt_max_workspace_size");
        values.emplace_back(trtMaxWorkSpaceSiz.c_str());
    }

    std::string deviceId = std::to_string(m_config->m_executionDeviceId);
    keys.emplace_back("device_id");
    values.emplace_back(deviceId.c_str());

    std::string enableTensorrtCache;
    std::string enableTensorrtEmbedEngine;
    std::string tensorrtEmbedEnginePath;
    if (m_config->m_enableTensorrtEmbedEngine) {
        enableTensorrtCache = std::to_string(m_config->m_enableTensorrtCache);
        enableTensorrtEmbedEngine = std::to_string(m_config->m_enableTensorrtEmbedEngine);
        tensorrtEmbedEnginePath = "./trt_engine_cache";

        keys.emplace_back("trt_engine_cache_enable");
        values.emplace_back(enableTensorrtCache.c_str());
        keys.emplace_back("trt_dump_ep_context_model");
        values.emplace_back(enableTensorrtEmbedEngine.c_str());
        keys.emplace_back("trt_ep_context_file_path");
        values.emplace_back(tensorrtEmbedEnginePath.c_str());
    }

    std::string tensorrtCachePath;
    if (m_config->m_enableTensorrtCache) {
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
        m_sessionOptions.AppendExecutionProvider_TensorRT_V2(*tensorrtProviderOptionsV2);
    } catch (const Ort::Exception &e) {
        m_logger->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
    } catch (const std::exception &e) {
        m_logger->warn(std::format("Failed to append TensorRT execution provider: {}", e.what()));
    } catch (...) {
        m_logger->error("Failed to append TensorRT execution provider: Unknown error");
    }
    api.ReleaseTensorRTProviderOptions(tensorrtProviderOptionsV2);
}
} // namespace Ffc
