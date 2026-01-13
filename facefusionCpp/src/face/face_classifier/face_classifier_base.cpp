/**
 ******************************************************************************
 * @file           : face_classifier_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

module face_classifier_hub;
import :face_classifier_base;

namespace ffc::face_classifier {

FaceClassifierBase::FaceClassifierBase(const std::shared_ptr<Ort::Env>& env) :
    InferenceSession(env) {}

} // namespace ffc::face_classifier