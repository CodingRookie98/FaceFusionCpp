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
                            const foundation::ai::inference_session::Options& options = {}) = 0;

    /**
     * @brief Restore expression from source to target
     * @param input Restoration parameters
     * @return Result image with expressions restored
     */
    virtual cv::Mat restore_expression(const RestoreExpressionInput& input) = 0;
};

} // namespace domain::face::expression
