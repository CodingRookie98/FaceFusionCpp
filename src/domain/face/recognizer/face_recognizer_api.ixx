/**
 * @file face_recognizer_api.ixx
 * @brief Face recognizer interface definition
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <vector>
#include <utility>
#include <memory>
#include <string>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module domain.face.recognizer:api;

import domain.face;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

namespace domain::face::recognizer {

/**
 * @brief Abstract base class for face recognizers
 */
export class FaceRecognizer {
public:
    virtual ~FaceRecognizer() = default;

    /**
     * @brief Load the recognition model
     * @param model_path Path to the model file
     * @param options Inference session options
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options) {
        m_session =
            foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->get_session(
                model_path, options);
    }

    /**
     * @brief Extract face embedding from a face image
     * @param vision_frame Input image (full frame)
     * @param face_landmark_5 5-point face landmarks for alignment
     * @return Pair of {raw_embedding, normalized_embedding}
     */
    virtual std::pair<types::Embedding, types::Embedding> recognize(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5) = 0;

    /**
     * @brief Check if a model is currently loaded
     * @return true if loaded, false otherwise
     */
    [[nodiscard]] bool is_model_loaded() const { return m_session && m_session->is_model_loaded(); }

    /**
     * @brief Get dimensions of input nodes
     * @return Vector of dimension vectors for each input node
     */
    [[nodiscard]] std::vector<std::vector<std::int64_t>> get_input_node_dims() const {
        if (!m_session) return {};
        return m_session->get_input_node_dims();
    }

    /**
     * @brief Get dimensions of output nodes
     * @return Vector of dimension vectors for each output node
     */
    [[nodiscard]] std::vector<std::vector<std::int64_t>> get_output_node_dims() const {
        if (!m_session) return {};
        return m_session->get_output_node_dims();
    }

    /**
     * @brief Run inference with raw tensors
     * @param input_tensors Vector of input tensors
     * @return Vector of output tensors
     */
    std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors) const {
        if (!m_session) return {};
        return m_session->run(input_tensors);
    }

protected:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
};

} // namespace domain::face::recognizer
