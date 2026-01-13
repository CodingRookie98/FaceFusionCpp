/**
 ******************************************************************************
 * @file           : face_recognizer_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module face_recognizer_hub;
import :face_recognizer_base;

namespace ffc::face_recognizer {
FaceRecognizerBase::FaceRecognizerBase(const std::shared_ptr<Ort::Env>& env) :
    InferenceSession(env) {}
} // namespace ffc::face_recognizer