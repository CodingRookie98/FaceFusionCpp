module;
#include <memory>

export module domain.face.recognizer:factory;

import :api;

namespace domain::face::recognizer {

export enum class FaceRecognizerType { ArcFace_w600k_r50 };

export std::unique_ptr<FaceRecognizer> create_face_recognizer(FaceRecognizerType type);

} // namespace domain::face::recognizer
