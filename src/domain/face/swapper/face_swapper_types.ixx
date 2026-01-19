module;
#include <vector>
#include <array>
#include <opencv2/core.hpp>

export module domain.face.swapper:types;

export import domain.face;

export namespace domain::face::swapper {

// using MaskType = domain::face::types::MaskType; // Redundant
// using MaskOptions = domain::face::types::MaskOptions; // Redundant

/**
 * @brief Input parameters for face swapping
 */
struct SwapInput {
    /// @brief Source face embedding vector
    domain::face::types::Embedding source_embedding;
    /// @brief Landmarks of target faces to swap
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    /// @brief Target video frame or image
    cv::Mat target_frame;
    /// @brief Options for mask generation
    domain::face::types::MaskOptions mask_options;
};

} // namespace domain::face::swapper
