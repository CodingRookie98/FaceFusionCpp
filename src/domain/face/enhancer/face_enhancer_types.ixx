module;
#include <vector>
#include <array>
#include <memory>
#include <unordered_set>
#include <opencv2/core.hpp>

export module domain.face.enhancer:types;
export import domain.face; // for domain::face::types::Landmarks
// import domain.face.masker; // Removed dependency to fix build

export namespace domain::face::enhancer {

// Redefine mask arguments locally to avoid dependency on internal FaceMaskerHub
// or old module structures.
// struct MaskOptions { ... } // Removed, using domain.face::types::MaskOptions

// For compatibility with old FaceMaskerHub::ArgsForGetBestMask
// We can map these options to whatever the Masker implementation needs
// using MaskType = domain::face::types::MaskType;
// using MaskOptions = domain::face::types::MaskOptions;

struct EnhanceInput {
    // domain.face:types is exported by domain.face
    std::vector<domain::face::types::Landmarks> target_faces_landmarks;
    cv::Mat target_frame;
    unsigned short face_blend = 80;

    domain::face::types::MaskOptions mask_options;
};

} // namespace domain::face::enhancer
