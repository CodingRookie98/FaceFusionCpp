module;
#include <vector>
#include <array>
#include <opencv2/opencv.hpp>

export module domain.face:types;

export namespace domain::face::types {
using Embedding = std::vector<float>;
using Landmarks = std::vector<cv::Point2f>;
using Score = float;

enum class MaskType { Box, Occlusion, Region };

struct MaskOptions {
    std::vector<MaskType> mask_types = {MaskType::Box};
    float box_mask_blur = 0.3f;
    std::array<int, 4> box_mask_padding = {0, 0, 0, 0};
};

struct FaceProcessResult {
    cv::Mat crop_frame;        // The swapped face
    cv::Mat target_crop_frame; // The original target face crop (for masking)
    cv::Mat affine_matrix;
    Landmarks target_landmarks;
    MaskOptions mask_options;
};

} // namespace domain::face::types
