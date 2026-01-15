module;
#include <vector>
#include <array>
#include <opencv2/core.hpp>

export module domain.face.swapper:types;

import domain.face;

export namespace domain::face::swapper {

enum class MaskType { Box, Occlusion, Region };

struct MaskOptions {
    std::vector<MaskType> mask_types = {MaskType::Box};
    float box_mask_blur = 0.3f;
    std::array<int, 4> box_mask_padding = {0, 0, 0, 0};
    // Region mask specific options could go here
    // Occlusion mask specific options could go here
};

struct SwapInput {
    domain::face::types::Embedding source_embedding;
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    cv::Mat target_frame;
    MaskOptions mask_options;
};

} // namespace domain::face::swapper
