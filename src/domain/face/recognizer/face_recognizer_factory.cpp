module;
#include <memory>

module domain.face.recognizer;

import :impl.arcface;

namespace domain::face::recognizer {

std::unique_ptr<FaceRecognizer> create_face_recognizer(FaceRecognizerType type) {
    switch (type) {
    case FaceRecognizerType::ArcFaceW600kR50: return std::make_unique<ArcFace>();
    default: return nullptr;
    }
}

} // namespace domain::face::recognizer
