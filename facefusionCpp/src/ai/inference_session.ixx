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

namespace ffc::ai {

using namespace infra;

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

        bool operator==(const Options& other) const {
            return execution_providers == other.execution_providers && execution_device_id == other.execution_device_id && trt_max_workspace_size == other.trt_max_workspace_size && enable_tensorrt_embed_engine == other.enable_tensorrt_embed_engine && enable_tensorrt_cache == other.enable_tensorrt_cache;
        }
    };

protected:
    std::unique_ptr<Ort::Session> m_ort_session;
    Ort::SessionOptions m_session_options;
    Ort::RunOptions m_run_options;
    std::unique_ptr<OrtCUDAProviderOptions> m_cuda_provider_options{nullptr};
    std::vector<const char*> m_input_names;
    std::vector<const char*> m_output_names;
    std::vector<std::vector<int64_t>> m_input_node_dims;  // >=1 outputs
    std::vector<std::vector<int64_t>> m_output_node_dims; // >=1 outputs
    std::unique_ptr<Ort::MemoryInfo> m_memory_info;

private:
    std::mutex m_mutex;
    std::shared_ptr<Ort::Env> m_ort_env;
    std::unordered_set<std::string> m_available_providers;
    Options m_options;
    std::vector<Ort::AllocatedStringPtr> m_input_names_ptrs;
    std::vector<Ort::AllocatedStringPtr> m_output_names_ptrs;
    std::shared_ptr<Logger> m_logger;
    bool m_is_model_loaded = false;
    std::string m_model_path;

public:
    virtual void load_model(const std::string& model_path, const Options& options);
    [[nodiscard]] bool is_model_loaded() const;
    [[nodiscard]] std::string get_loaded_model_path() const;

private:
    void append_provider_cuda();
    void append_provider_tensorrt();
};

} // namespace ffc::ai
