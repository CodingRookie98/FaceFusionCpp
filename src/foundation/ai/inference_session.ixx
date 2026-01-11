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

export module foundation.ai.inference_session;

namespace foundation::ai::inference_session {

    export enum class ExecutionProvider {
        CPU,     ///< CPU execution provider
        CUDA,    ///< CUDA GPU execution provider
        TensorRT ///< TensorRT execution provider
    };

    export struct Options {
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
 * @brief ONNX Runtime inference session wrapper class
 * @details This class provides a high-level interface for loading ONNX models and running
 *          inference with support for various execution providers including CPU, CUDA, and TensorRT.
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
     * @param model_path Path to the ONNX model file
     * @param options Session options including execution providers and device configuration
     * @note This method will reset the current session state before loading the new model.
     *       If the same model is already loaded with the same options, it will not be reloaded.
     */
    void load_model(const std::string& model_path, const Options& options);

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

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace foundation::ai::inference_session
