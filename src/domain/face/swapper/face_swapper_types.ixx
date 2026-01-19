module;
#include <vector>
#include <array>
#include <opencv2/core.hpp>

export module domain.face.swapper:types;

export import domain.face;

export namespace domain::face::swapper {

// using MaskType = domain::face::types::MaskType; // Redundant
// using MaskOptions = domain::face::types::MaskOptions; // Redundant

struct SwapInput {
    domain::face::types::Embedding source_embedding;
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    cv::Mat target_frame;
    domain::face::types::MaskOptions mask_options;
};

} // namespace domain::face::swapper
