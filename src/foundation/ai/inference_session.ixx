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
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <onnxruntime_cxx_api.h>

export module foundation.ai.inference_session;

namespace foundation::ai::inference_session {

export enum class ExecutionProvider {
    CPU,     ///< CPU execution provider
    CUDA,    ///< CUDA GPU execution provider
    TensorRT ///< TensorRT execution provider
};

/**
 * @brief Get the best available execution providers (TensorRT > CUDA > CPU)
 * @return std::unordered_set<ExecutionProvider> Set of available providers in priority order
 */
export std::unordered_set<ExecutionProvider> get_best_available_providers();

export struct Options {
    std::unordered_set<ExecutionProvider> execution_providers{}; ///< Empty = auto-detect best
    int execution_device_id = 0;                                 ///< Device ID for GPU execution
    size_t trt_max_workspace_size = 0;        ///< Maximum workspace size for TensorRT in GB
    bool enable_tensorrt_embed_engine = true; ///< Enable TensorRT engine embedding
    bool enable_tensorrt_cache = true;        ///< Enable TensorRT engine caching

    bool operator==(const Options& other) const {
        return execution_providers == other.execution_providers
            && execution_device_id == other.execution_device_id
            && trt_max_workspace_size == other.trt_max_workspace_size
            && enable_tensorrt_embed_engine == other.enable_tensorrt_embed_engine
            && enable_tensorrt_cache == other.enable_tensorrt_cache;
    }

    /**
     * @brief Create options with best available providers (TensorRT > CUDA > CPU)
     */
    static Options with_best_providers() {
        Options opts;
        opts.execution_providers = get_best_available_providers();
        return opts;
    }
};

/**
 * @brief ONNX Runtime inference session wrapper class
 * @details This class provides a high-level interface for loading ONNX models and running
 *          inference with support for various execution providers including CPU, CUDA, and
 * TensorRT.
 */
export class InferenceSession {
public:
    /**
     * @brief Construct an InferenceSession object
     */
    InferenceSession();

    virtual ~InferenceSession();

    /**
     * @brief Load an ONNX model with specified options
     * @param model_path Path to the
     * ONNX model file
     * @param options Session options including execution providers and
     * device configuration
     * @note This method will reset the current session state before
     * loading the new model.
     *       If the same model is already loaded with the same
     * options, it will not be reloaded.
     */
    virtual void load_model(const std::string& model_path, const Options& options);

    /**
     * @brief Check if a model is currently loaded

     * @return true if a model is
     * loaded, false otherwise
     */
    [[nodiscard]] bool is_model_loaded() const;

    /**
     * @brief Get the path of the currently loaded model
     * @return Path to the loaded model file, or empty string if no model is loaded
     */
    [[nodiscard]] std::string get_loaded_model_path() const;

    /**
     * @brief Run inference
     * @param input_tensors Vector of input tensors
     *
     * @return Vector of output tensors
     */
    virtual std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors);

    [[nodiscard]] std::vector<std::vector<int64_t>> get_input_node_dims() const;

    [[nodiscard]] std::vector<std::vector<int64_t>> get_output_node_dims() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Registry for sharing InferenceSession instances.
 * @details This class ensures that
 * inference sessions are shared across different
 *          components if they use the same model
 * path and options.
 */
export class InferenceSessionRegistry {
public:
    /**
     * @brief Get the singleton instance of the registry.
     */
    static InferenceSessionRegistry& get_instance();

    /**
     * @brief Get a shared inference session.
     * @param model_path Path to the ONNX
     * model file.
     * @param options Session options.
     * @return A shared pointer to an
     * InferenceSession.
     */
    std::shared_ptr<InferenceSession> get_session(const std::string& model_path,
                                                  const Options& options);

    /**
     * @brief Clear all cached sessions.
     */
    void clear();

private:
    InferenceSessionRegistry() = default;
    ~InferenceSessionRegistry() = default;

    InferenceSessionRegistry(const InferenceSessionRegistry&) = delete;
    InferenceSessionRegistry& operator=(const InferenceSessionRegistry&) = delete;

    std::mutex m_mutex;
    std::unordered_map<std::string, std::weak_ptr<InferenceSession>> m_sessions;

    std::string generate_key(const std::string& model_path, const Options& options);
};

} // namespace foundation::ai::inference_session
