module;
#include <vector>
#include <array>
#include <opencv2/core.hpp>

export module domain.face.swapper:types;

export import domain.face;

export namespace domain::face::swapper {

/**
 * @brief Input parameters for face swapping (DTO for Pipeline)
 */
struct SwapInput {
    /// @brief Source face embedding vector
    domain::face::types::Embedding source_embedding;
    /// @brief Landmarks of target faces to swap
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    /// @brief Options for mask generation
    domain::face::types::MaskOptions mask_options;
};

} // namespace domain::face::swapper
