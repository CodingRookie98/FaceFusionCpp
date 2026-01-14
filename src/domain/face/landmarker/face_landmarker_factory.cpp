module;
#include <memory>

module domain.face.landmarker;

import :impl;

namespace domain::face::landmarker {

std::unique_ptr<IFaceLandmarker> create_landmarker(LandmarkerType type) {
    switch (type) {
    case LandmarkerType::_2DFAN: return std::make_unique<T2dfan>();
    case LandmarkerType::Peppawutz: return std::make_unique<Peppawutz>();
    case LandmarkerType::_68By5: return std::make_unique<T68By5>();
    default: return nullptr;
    }
}

} // namespace domain::face::landmarker
