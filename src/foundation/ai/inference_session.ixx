/**
 * @file inference_session.ixx
 * @brief ONNX Runtime inference session wrapper module
 * @author CodingRookie
 * @date 2024-07-12
 * @note This module provides a wrapper for ONNX Runtime inference sessions with support for
 *       multiple execution providers (CPU, CUDA, TensorRT).
 */

module;
#include <unordered_set>
#include <mutex>
#include <onnxruntime_cxx_api.h>

export module foundation.ai.inference_session;
import foundation.infrastructure.logger;

namespace foundation::ai::inference_session {

using namespace foundation::infrastructure; // Adapt based on actual logger namespace usage

/**
 * @brief ONNX Runtime inference session wrapper class
 * @details This class provides a high-level interface for loading ONNX models and running
 *          inference with support for various execution providers including CPU, CUDA, and TensorRT.
 */
export class InferenceSession {
public:
    enum class ExecutionProvider {
        CPU,     ///< CPU execution provider
        CUDA,    ///< CUDA GPU execution provider
        TensorRT ///< TensorRT execution provider
    };

    struct Options {
        std::unordered_set<ExecutionProvider> execution_providers{ExecutionProvider::CPU}; ///< Set of execution providers to use
        int execution_device_id = 0;                                                       ///< Device ID for GPU execution
        size_t trt_max_workspace_size = 0;                                                 ///< Maximum workspace size for TensorRT in GB
        bool enable_tensorrt_embed_engine = true;                                          ///< Enable TensorRT engine embedding
        bool enable_tensorrt_cache = true;                                                 ///< Enable TensorRT engine caching

        bool operator==(const Options& other) const {
            return execution_providers == other.execution_providers && execution_device_id == other.execution_device_id && trt_max_workspace_size == other.trt_max_workspace_size && enable_tensorrt_embed_engine == other.enable_tensorrt_embed_engine && enable_tensorrt_cache == other.enable_tensorrt_cache;
        }
    };

    /**
     * @brief Construct an InferenceSession object
     * @param env Optional ONNX Runtime environment. If nullptr, a new environment will be created.
     */
    explicit InferenceSession(const std::shared_ptr<Ort::Env>& env = nullptr);

    virtual ~InferenceSession() = default;

    /**
     * @brief Load an ONNX model with specified options
     * @param model_path Path to the ONNX model file
     * @param options Session options including execution providers and device configuration
     * @note This method will reset the current session state before loading the new model.
     *       If the same model is already loaded with the same options, it will not be reloaded.
     */
    virtual void load_model(const std::string& model_path, const Options& options);

    /**
     * @brief Check if a model is currently loaded
     * @return true if a model is loaded, false otherwise
     */
    [[nodiscard]] bool is_model_loaded() const;

    /**
     * @brief Get the path of the currently loaded model
     * @return Path to the loaded model file, or empty string if no model is loaded
     */
    [[nodiscard]] std::string get_loaded_model_path() const;

protected:
    std::unique_ptr<Ort::Session> m_ort_session;                              ///< ONNX Runtime session object
    Ort::SessionOptions m_session_options;                                    ///< Session configuration options
    Ort::RunOptions m_run_options;                                            ///< Run configuration options
    std::unique_ptr<OrtCUDAProviderOptions> m_cuda_provider_options{nullptr}; ///< CUDA execution provider options
    std::vector<const char*> m_input_names;                                   ///< Input tensor names
    std::vector<const char*> m_output_names;                                  ///< Output tensor names
    std::vector<std::vector<int64_t>> m_input_node_dims;                      ///< Input tensor dimensions
    std::vector<std::vector<int64_t>> m_output_node_dims;                     ///< Output tensor dimensions
    std::unique_ptr<Ort::MemoryInfo> m_memory_info;                           ///< Memory information for tensor allocation

private:
    std::mutex m_mutex;                                       ///< Mutex for thread-safe operations
    std::shared_ptr<Ort::Env> m_ort_env;                      ///< ONNX Runtime environment
    std::unordered_set<std::string> m_available_providers;    ///< Available execution providers
    Options m_options;                                        ///< Current session options
    std::vector<Ort::AllocatedStringPtr> m_input_names_ptrs;  ///< Allocated input name strings
    std::vector<Ort::AllocatedStringPtr> m_output_names_ptrs; ///< Allocated output name strings
    std::shared_ptr<logger::Logger> m_logger;                                              ///< Logger instance
    bool m_is_model_loaded = false;                           ///< Flag indicating if model is loaded
    std::string m_model_path;                                 ///< Path to the loaded model

    /**
     * @brief Append CUDA execution provider to session options
     * @note This method configures CUDA provider options including device ID and registers CUDA kernels.
     *       It is called internally during model loading if CUDA is specified in the execution providers.
     */
    void append_provider_cuda();

    /**
     * @brief Append TensorRT execution provider to session options
     * @note This method configures TensorRT provider options including workspace size, engine embedding,
     *       and caching. It is called internally during model loading if TensorRT is specified in the execution providers.
     */
    void append_provider_tensorrt();

    /**
     * @brief Reset the session state
     * @note This method clears all session state including the loaded model, input/output metadata,
     *       and resets session options. It is called internally before loading a new model.
     */
    void reset();
};

} // namespace foundation::ai::inference_session
