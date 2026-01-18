module;
#include <opencv2/core/mat.hpp>
#include <string>
#include <vector>

/**
 * @file face_detector_api.ixx
 * @brief Face detector interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.detector:api;

import :types;
import foundation.ai.inference_session;

export namespace domain::face::detector {

using InferenceOptions = foundation::ai::inference_session::Options;

/**
 * @brief Interface for Face Detectors
 */
class IFaceDetector {
public:
    virtual ~IFaceDetector() = default;

    /**
     * @brief Load the detection model
     * @param model_path Path to the model file
     * @param options Inference session options
     */
    virtual void load_model(const std::string& model_path,
                            const InferenceOptions& options = {}) = 0;

    /**
     * @brief Detect faces in the given image.
     * @param image Input image (BGR usually)
     * @return List of detection results
     */
    virtual DetectionResults detect(const cv::Mat& image) = 0;
};
} // namespace domain::face::detector
