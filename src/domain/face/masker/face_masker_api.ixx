module;
#include <opencv2/core/mat.hpp>
#include <unordered_set>

export module domain.face.masker:api;

export namespace domain::face::masker {

/**
 * @brief Face regions for semantic segmentation
 */
enum class FaceRegion {
    Background,
    Skin,
    LeftEyebrow,
    RightEyebrow,
    LeftEye,
    RightEye,
    EyeGlasses,
    LeftEar,
    RightEar,
    Earring,
    Nose,
    Mouth,
    UpperLip,
    LowerLip,
    Neck,
    Necklace,
    Cloth,
    Hair,
    Hat
};

/**
 * @brief Interface for face occlusion masker
 * Creates a mask where occlusions (hands, objects) are white (255)
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
 * Creates a mask for specific face regions
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
