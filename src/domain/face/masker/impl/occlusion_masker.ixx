/**
 * @file occlusion_masker.ixx
 * @brief Concrete implementation of Face Occlusion Masker
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core/mat.hpp>
#include <vector>

export module domain.face.masker:impl_occlusion;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

/**
 * @brief Implementation of IFaceOccluder that detects face occlusions
 * @details Inherits from InferenceSession to handle model execution
 */
class OcclusionMasker final : public IFaceOccluder,
                              public foundation::ai::inference_session::InferenceSession {
public:
    using foundation::ai::inference_session::InferenceSession::InferenceSession;
    ~OcclusionMasker() override = default;

    /**
     * @brief Create an occlusion mask from a face crop
     * @param crop_vision_frame Input face crop (BGR)
     * @return Single channel mask (CV_8UC1), 255=Occluded, 0=Clear
     */
    cv::Mat create_occlusion_mask(const cv::Mat& crop_vision_frame) override;
};

} // namespace domain::face::masker
