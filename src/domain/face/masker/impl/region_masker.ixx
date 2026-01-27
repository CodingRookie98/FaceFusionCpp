/**
 * @file region_masker.ixx
 * @brief Concrete implementation of Face Region Masker (Face Parsing)
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core/mat.hpp>
#include <unordered_set>
#include <vector>

export module domain.face.masker:impl_region;

import :api;
import foundation.ai.inference_session;

export namespace domain::face::masker {

/**
 * @brief Implementation of IFaceRegionMasker using semantic segmentation
 * @details Inherits from InferenceSession to handle model execution
 */
class RegionMasker final : public IFaceRegionMasker,
                           public foundation::ai::inference_session::InferenceSession {
public:
    using foundation::ai::inference_session::InferenceSession::InferenceSession;
    ~RegionMasker() override = default;

    /**
     * @brief Create a region mask for selected regions
     * @param crop_vision_frame Input face crop (BGR)
     * @param regions Set of regions to include in the mask
     * @return Single channel mask (CV_8UC1), 255=Selected Region, 0=Other
     */
    cv::Mat create_region_mask(const cv::Mat& crop_vision_frame,
                               const std::unordered_set<FaceRegion>& regions) override;
};

} // namespace domain::face::masker
