/**
 ******************************************************************************
 * @file           : face_masker_base.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>
#include "common_macros.h" // don't remove this line

// import face_masker.face_masker_base;
module face_masker_hub;
import :face_masker_base;

namespace ffc::faceMasker {
FaceMaskerBase::FaceMaskerBase(const std::shared_ptr<Ort::Env> &env) :
    InferenceSession(env) {
}
} // namespace ffc::faceMasker