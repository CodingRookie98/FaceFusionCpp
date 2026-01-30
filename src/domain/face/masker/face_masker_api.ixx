/**
 * @file face_masker_api.ixx
 * @brief Interfaces for face masking and occlusion detection
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <opencv2/core/mat.hpp>
#include <unordered_set>

export module domain.face.masker:api;

import domain.face;

export namespace domain::face::masker {

using domain::face::types::FaceRegion;

/**
 * @brief Interface for face occlusion masker
 * @details Creates a mask where occlusions (hands, objects) are white (255)
 */
class IFaceOccluder {
public:
    virtual ~IFaceOccluder() = default;

    /**
     * @brief Create an occlusion mask from a face crop
     * @param crop_vision_frame Input face crop (BGR)
     * @return Single channel mask (CV_8UC1), 255=Occluded, 0=Clear
     */
    [[nodiscard]] virtual cv::Mat create_occlusion_mask(const cv::Mat& crop_vision_frame) = 0;
};

/**
 * @brief Interface for face region masker (Face Parsing)
 * @details Creates a mask for specific face regions
 */
class IFaceRegionMasker {
public:
    virtual ~IFaceRegionMasker() = default;

    /**
     * @brief Create a region mask for selected regions
     * @param crop_vision_frame Input face crop (BGR)
     * @param regions Set of regions to include in the mask
     * @return Single channel mask (CV_8UC1), 255=Selected Region, 0=Other
     */
    [[nodiscard]] virtual cv::Mat create_region_mask(
        const cv::Mat& crop_vision_frame, const std::unordered_set<FaceRegion>& regions) = 0;
};

} // namespace domain::face::masker
