/**
 * @file face_types.ixx
 * @brief Common types for face domain
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <vector>
#include <array>
#include <opencv2/opencv.hpp>

export module domain.face:types;

export namespace domain::face::types {

/**
 * @brief Face embedding vector
 */
using Embedding = std::vector<float>;

/**
 * @brief Facial landmarks (2D points)
 */
using Landmarks = std::vector<cv::Point2f>;

/**
 * @brief Confidence score
 */
using Score = float;

/**
 * @brief Types of masks for face processing
 */
enum class MaskType {
    Box,       ///< Bounding box based mask
    Occlusion, ///< Detected occlusion based mask
    Region     ///< Semantic region based mask (Face Parsing)
};

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
 * @brief Configuration for face masking
 */
struct MaskOptions {
    std::vector<MaskType> mask_types = {MaskType::Box}; ///< Active mask types
    std::vector<FaceRegion> regions = {
        FaceRegion::Skin,         FaceRegion::Nose,     FaceRegion::LeftEyebrow,
        FaceRegion::RightEyebrow, FaceRegion::LeftEye,  FaceRegion::RightEye,
        FaceRegion::Mouth,        FaceRegion::UpperLip, FaceRegion::LowerLip}; ///< Active regions
    float box_mask_blur = 0.3f; ///< Blur intensity for box mask
    std::array<int, 4> box_mask_padding = {0, 0, 0,
                                           0}; ///< Padding for box mask (top, right, bottom, left)
};

/**
 * @brief Result of a face processing step (e.g., swapping or enhancement)
 */
struct FaceProcessResult {
    cv::Mat crop_frame;         ///< Processed face crop
    cv::Mat target_crop_frame;  ///< Original face crop from target image
    cv::Mat affine_matrix;      ///< Transformation matrix used for alignment
    Landmarks target_landmarks; ///< Landmarks of the target face
    MaskOptions mask_options;   ///< Mask configuration for pasting back
};

} // namespace domain::face::types
