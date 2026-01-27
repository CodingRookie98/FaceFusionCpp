/**
 * @file face_recognizer_factory.ixx
 * @brief Factory for creating Face Recognizer instances
 * @author CodingRookie
 * @date 2026-01-27
 */
module;
#include <memory>

export module domain.face.recognizer:factory;

import :api;

namespace domain::face::recognizer {

/**
 * @brief Available types of Face Recognizers
 */
export enum class FaceRecognizerType {
    ArcFace_w600k_r50 ///< ArcFace model trained on MS1MV3 (600k identities, ResNet-50)
};

/**
 * @brief Create a Face Recognizer instance
 * @param type Type of Face Recognizer to create
 * @return std::unique_ptr<FaceRecognizer> Created face recognizer instance
 */
export std::unique_ptr<FaceRecognizer> create_face_recognizer(FaceRecognizerType type);

} // namespace domain::face::recognizer
