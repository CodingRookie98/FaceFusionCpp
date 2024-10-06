/**
 ******************************************************************************
 * @file           : ort_session.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-12
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_INFERENCE_SESSION_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_INFERENCE_SESSION_H_

#include <mutex>
#include <string>
#include <unordered_set>
#include <onnxruntime_cxx_api.h>
#include "logger.h"

namespace Ffc {
class Config; // 避免循环引用
class InferenceSession {
public:
    explicit InferenceSession(const std::shared_ptr<Ort::Env> &env);
    ~InferenceSession() = default;

    enum ExecutionProvider {
        CPU,
        CUDA,
        TensorRT
    };

    void createSession(const std::string &modelPath);

    std::mutex m_mutex;
    std::shared_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_ortSession;
    Ort::SessionOptions m_sessionOptions;
    Ort::RunOptions m_runOptions;
    std::shared_ptr<OrtCUDAProviderOptions> m_cudaProviderOptions = nullptr;
    std::shared_ptr<OrtTensorRTProviderOptions> m_tensorrtProviderOptions = nullptr;
    std::vector<const char *> m_inputNames;
    std::vector<const char *> m_outputNames;
    std::vector<Ort::AllocatedStringPtr> m_inputNamesPtrs;
    std::vector<Ort::AllocatedStringPtr> m_outputNamesPtrs;
    std::vector<std::vector<int64_t>> m_inputNodeDims;  // >=1 outputs
    std::vector<std::vector<int64_t>> m_outputNodeDims; // >=1 outputs
    Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
    std::shared_ptr<Ffc::Config> m_config;
    std::unordered_set<std::string> m_availableProviders;
    std::shared_ptr<Ffc::Logger> m_logger = Ffc::Logger::getInstance();

private:
    void appendProviderCUDA();
    void appendProviderTensorrt();
};

} // namespace Ffc

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_INFERENCE_SESSION_H_
