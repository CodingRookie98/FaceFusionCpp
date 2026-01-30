/**
 * @file inference_session_registry.ixx
 * @brief Registry for sharing InferenceSession instances
 * @author CodingRookie
 * @date 2024-07-12
 */

module;
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

export module foundation.ai.inference_session_registry;

import foundation.ai.inference_session;
import foundation.ai.session_pool;

namespace foundation::ai::inference_session {

/**
 * @brief Registry for sharing InferenceSession instances.
 * @details This class ensures that inference sessions are shared across different
 *          components if they use the same model path and options.
 */
export class InferenceSessionRegistry {
public:
    /**
     * @brief Get the singleton instance of the registry.
     * @return Reference to the singleton instance
     */
    static InferenceSessionRegistry& get_instance();

    /**
     * @brief Get a shared inference session.
     * @param model_path Path to the ONNX model file.
     * @param options Session options.
     * @return A shared pointer to an InferenceSession.
     */
    std::shared_ptr<InferenceSession> get_session(const std::string& model_path,
                                                  const Options& options);

    /**
     * @brief Clear all cached sessions.
     */
    void clear();

    /**
     * @brief Trigger cleanup of expired sessions.
     * @return Number of cleaned sessions.
     */
    size_t cleanup_expired();

private:
    InferenceSessionRegistry();
    ~InferenceSessionRegistry() = default;

    InferenceSessionRegistry(const InferenceSessionRegistry&) = delete;
    InferenceSessionRegistry& operator=(const InferenceSessionRegistry&) = delete;

    session_pool::SessionPool m_pool;

    /**
     * @brief Generate a unique key for a model and options combination
     */
    std::string generate_key(const std::string& model_path, const Options& options);
};

} // namespace foundation::ai::inference_session
