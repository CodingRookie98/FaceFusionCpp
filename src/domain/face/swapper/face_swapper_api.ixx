module;
#include <string>
#include <vector>
#include <opencv2/core.hpp>

/**
 * @file face_swapper_api.ixx
 * @brief Face swapper interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.swapper:api;

import :types;
import domain.face;
import foundation.ai.inference_session;

export namespace domain::face::swapper {

/**
 * @brief Interface for Face Swappers
 */
class IFaceSwapper {
public:
    virtual ~IFaceSwapper() = default;

    /**
     * @brief Load the swapper model
     * @param model_path Path to the ONNX model
     * @param options Inference options (device, threads, etc.)
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief Perform face swapping on a single aligned face crop
     * @param target_crop Aligned 128x128 face crop
     * @param source_embedding Source face embedding
     * @return Swapped face crop
     */
    virtual cv::Mat swap_face(cv::Mat target_crop, std::vector<float> source_embedding) = 0;

    /**
     * @brief Get the expected input size for the model
     * @return cv::Size (e.g., 128x128)
     */
    virtual cv::Size get_model_input_size() const = 0;
};

} // namespace domain::face::swapper
