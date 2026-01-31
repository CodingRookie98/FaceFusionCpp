module;
#include <string>
#include <vector>
#include <opencv2/core.hpp>

/**
 * @file face_enhancer_api.ixx
 * @brief Face enhancer interface definition
 * @author CodingRookie
 * @date 2026-01-18
 */
export module domain.face.enhancer:api;
import :types;
import domain.face;
import foundation.ai.inference_session;

export namespace domain::face::enhancer {

/**
 * @brief Interface for Face Enhancers (restoration)
 */
class IFaceEnhancer {
public:
    virtual ~IFaceEnhancer() = default;

    /**
     * @brief Load the enhancer model
     * @param model_path Path to the model file
     * @param options Inference session options
     */
    virtual void load_model(const std::string& model_path,
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief Enhance/Restore a single face crop
     * @param target_crop Aligned 512x512 face crop
     * @return Enhanced face crop
     */
    virtual cv::Mat enhance_face(const cv::Mat& target_crop) = 0;

    /**
     * @brief Get the expected input size for the model
     * @return cv::Size
     */
    virtual cv::Size get_model_input_size() const = 0;
};

} // namespace domain::face::enhancer
