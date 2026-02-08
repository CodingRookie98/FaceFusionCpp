module;
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <opencv2/core.hpp>

export module domain.face.enhancer:types;
export import domain.face;

export namespace domain::face::enhancer {

/**
 * @brief Input parameters for face enhancement (DTO for Pipeline)
 */
struct EnhanceInput {
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    std::uint16_t face_blend = 80;
    domain::face::types::MaskOptions mask_options;
};

} // namespace domain::face::enhancer
