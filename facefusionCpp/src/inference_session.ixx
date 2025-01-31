/**
 ******************************************************************************
 * @file           : ort_session.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-12
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <mutex>
#include <onnxruntime_cxx_api.h>

export module inference_session;
import logger;

namespace ffc {
export class InferenceSession {
public:
    explicit InferenceSession(const std::shared_ptr<Ort::Env> &env = nullptr);
    virtual ~InferenceSession() = default;

    enum class ExecutionProvider {
        CPU,
        CUDA,
        TensorRT
    };

    struct Options {
        std::unordered_set<ExecutionProvider> executionProviders{ExecutionProvider::CPU};
        int executionDeviceId = 0;
        size_t trtMaxWorkspaceSize = 0;
        bool enableTensorrtEmbedEngine = true;
        bool enableTensorrtCache = true;
    };

    virtual void loadModel(const std::string &modelPath, const Options &options);
    [[nodiscard]] bool isModelLoaded() const;
    [[nodiscard]] std::string getModelPath() const;

protected:
    std::unique_ptr<Ort::Session> m_ortSession;
    Ort::SessionOptions m_sessionOptions;
    Ort::RunOptions m_runOptions;
    std::unique_ptr<OrtCUDAProviderOptions> m_cudaProviderOptions = nullptr;
    std::vector<const char *> m_inputNames;
    std::vector<const char *> m_outputNames;
    std::vector<std::vector<int64_t>> m_inputNodeDims;  // >=1 outputs
    std::vector<std::vector<int64_t>> m_outputNodeDims; // >=1 outputs
    Ort::MemoryInfo m_memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

private:
    void appendProviderCUDA();
    void appendProviderTensorrt();

    std::mutex m_mutex;
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_set<std::string> m_availableProviders;
    Options m_options;
    std::vector<Ort::AllocatedStringPtr> m_inputNamesPtrs;
    std::vector<Ort::AllocatedStringPtr> m_outputNamesPtrs;
    std::shared_ptr<Logger> m_logger;;
    bool m_isModelLoaded = false;
    std::string m_modelPath;
};

} // namespace ffc
