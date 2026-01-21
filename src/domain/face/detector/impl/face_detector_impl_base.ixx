module;
#include <opencv2/core/mat.hpp>
#include <onnxruntime_cxx_api.h>
#include <memory>
#include <string>
#include <vector>

/**
 * @file face_detector_impl_base.ixx
 * @brief Face detector implementation base class interface
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.detector:impl_base;

import :api;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;

export namespace domain::face::detector {

/**
 * @brief Base implementation class for Face Detectors
 * @details Provides common functionality for ONNX-based face detection models.
 *          Subclasses (YOLO, SCRFD, RetinaFace) implement the specific `detect()` method.
 */
class FaceDetectorImplBase : public IFaceDetector {
public:
    virtual ~FaceDetectorImplBase() = default;

    /**
     * @brief Load the detection model
     * @param model_path Path to the ONNX model file
     * @param options Inference session options
     */
    void load_model(const std::string& model_path, const InferenceOptions& options) override;

    /**
     * @brief Check if the model is loaded
     * @return True if a model is loaded and ready
     */
    [[nodiscard]] bool is_model_loaded() const;

    /**
     * @brief Get input tensor dimensions
     * @return Vector of dimension vectors for each input
     */
    [[nodiscard]] std::vector<std::vector<int64_t>> get_input_node_dims() const;

    /**
     * @brief Get output tensor dimensions
     * @return Vector of dimension vectors for each output
     */
    [[nodiscard]] std::vector<std::vector<int64_t>> get_output_node_dims() const;

    /**
     * @brief Run inference on input tensors
     * @param input_tensors Vector of input ONNX tensors
     * @return Vector of output ONNX tensors
     */
    std::vector<Ort::Value> run(const std::vector<Ort::Value>& input_tensors);

protected:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
};
} // namespace domain::face::detector
