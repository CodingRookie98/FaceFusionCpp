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
    explicit InferenceSession(const std::shared_ptr<Ort::Env>& env = nullptr);
    virtual ~InferenceSession() = default;

    enum class ExecutionProvider {
        CPU,
        CUDA,
        TensorRT
    };

    struct Options {
        std::unordered_set<ExecutionProvider> execution_providers{ExecutionProvider::CPU};
        int execution_device_id = 0;
        size_t trt_max_workspace_size = 0;
        bool enable_tensorrt_embed_engine = true;
        bool enable_tensorrt_cache = true;
    };

    virtual void LoadModel(const std::string& model_path, const Options& options);
    [[nodiscard]] bool IsModelLoaded() const;
    [[nodiscard]] std::string GetModelPath() const;

protected:
    std::unique_ptr<Ort::Session> ort_session_;
    Ort::SessionOptions session_options_;
    Ort::RunOptions run_options_;
    std::unique_ptr<OrtCUDAProviderOptions> cuda_provider_options_{nullptr};
    std::vector<const char*> input_names_;
    std::vector<const char*> output_names_;
    std::vector<std::vector<int64_t>> input_node_dims_;  // >=1 outputs
    std::vector<std::vector<int64_t>> output_node_dims_; // >=1 outputs
    std::unique_ptr<Ort::MemoryInfo> memory_info_;

private:
    void AppendProviderCuda();
    void AppendProviderTensorrt();

    std::mutex mutex_;
    std::shared_ptr<Ort::Env> ort_env_;
    std::unordered_set<std::string> available_providers_;
    Options options_;
    std::vector<Ort::AllocatedStringPtr> input_names_ptrs_;
    std::vector<Ort::AllocatedStringPtr> output_names_ptrs_;
    std::shared_ptr<Logger> logger_;
    bool is_model_loaded_ = false;
    std::string model_path_;
};

} // namespace ffc
