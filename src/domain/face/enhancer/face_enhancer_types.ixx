module;
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <opencv2/core.hpp>

export module domain.face.enhancer:types;
import domain.face; // for domain::face::types::Landmarks
// import domain.face.masker; // Removed dependency to fix build

export namespace domain::face::enhancer {

enum class MaskType { Box, Occlusion, Region };

// Redefine mask arguments locally to avoid dependency on internal FaceMaskerHub
// or old module structures.
struct MaskOptions {
    std::unordered_set<MaskType> mask_types = {MaskType::Box};
    float box_mask_blur = 0.5f;
    std::array<int, 4> box_mask_padding = {0, 0, 0, 0};

    // For compatibility with old FaceMaskerHub::ArgsForGetBestMask
    // We can map these options to whatever the Masker implementation needs
};

struct EnhanceInput {
    // domain.face:types is exported by domain.face
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    cv::Mat target_frame;
    unsigned short face_blend = 80;

    MaskOptions mask_options;
};

} // namespace domain::face::enhancer
