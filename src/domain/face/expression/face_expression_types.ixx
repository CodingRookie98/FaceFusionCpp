module;
#include <opencv2/core.hpp>
#include <vector>
#include <array>
#include <unordered_set>

export module domain.face.expression:types;

import domain.face;

export namespace domain::face::expression {

// Use common MaskType
using MaskType = domain::face::types::MaskType;

struct RestoreExpressionInput {
    cv::Mat source_frame;
    std::vector<domain::face::types::Landmarks> source_landmarks;
    // target_frame removed, handled by pipeline
    std::vector<domain::face::types::Landmarks> target_landmarks;
    float restore_factor{0.96f};

    // Masking options
    std::unordered_set<MaskType> mask_types{MaskType::Box};
    float box_mask_blur{0.5f};
    std::array<int, 4> box_mask_padding{0, 0, 0, 0};
};

} // namespace domain::face::expression
