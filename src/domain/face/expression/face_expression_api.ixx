module;
#include <string>
#include <opencv2/core.hpp>

export module domain.face.expression:api;

import :types;
import foundation.ai.inference_session;

export namespace domain::face::expression {

class IFaceExpressionRestorer {
public:
    virtual ~IFaceExpressionRestorer() = default;

    /**
     * @brief Load the expression restorer models
     * @param feature_extractor_path Path to the feature extractor ONNX model
     * @param motion_extractor_path Path to the motion extractor ONNX model
     * @param generator_path Path to the generator ONNX model
     * @param options Inference options (device, threads, etc.)
     */
    virtual void load_model(const std::string& feature_extractor_path,
                            const std::string& motion_extractor_path,
                            const std::string& generator_path,
                            const foundation::ai::inference_session::Options& options) = 0;

    /**
     * @brief Restore expression from source crop to target crop
     * @param source_crop Aligned source face crop
     * @param target_crop Aligned target face crop
     * @param restore_factor Restoration factor (0.0 - 1.0)
     * @return Result crop with expressions restored
     */
    virtual cv::Mat restore_expression(cv::Mat source_crop, cv::Mat target_crop,
                                       float restore_factor) = 0;

    /**
     * @brief Get the expected input size for the model
     * @return cv::Size
     */
    [[nodiscard]] virtual cv::Size get_model_input_size() const = 0;
};

} // namespace domain::face::expression
