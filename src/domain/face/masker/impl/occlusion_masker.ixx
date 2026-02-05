/**
 * @file occlusion_masker.ixx
 * @brief Concrete implementation of Face Occlusion Masker
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core/mat.hpp>
#include <vector>
#include <memory>
#include <string>

export module domain.face.masker:impl_occlusion;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

/**
 * @brief Implementation of IFaceOccluder that detects face occlusions
 */
class OcclusionMasker final : public IFaceOccluder {
public:
    OcclusionMasker() = default;
    ~OcclusionMasker() override = default;

    /**
     * @brief Load the occlusion detection model
     * @param model_path Path to the ONNX model
     * @param options Inference options
     */
    void load_model(const std::string& model_path,
                    const foundation::ai::inference_session::Options& options);

    /**
     * @brief Create an occlusion mask from a face crop
     * @param crop_vision_frame Input face crop (BGR)
     * @return Single channel mask (CV_8UC1), 255=Occluded, 0=Clear
     */
    cv::Mat create_occlusion_mask(const cv::Mat& crop_vision_frame) override;

private:
    std::shared_ptr<foundation::ai::inference_session::InferenceSession> m_session;
};

} // namespace domain::face::masker
