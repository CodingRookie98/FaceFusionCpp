/**
 ******************************************************************************
 * @file           : face_masker_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <onnxruntime_cxx_api.h>

export module face_masker_hub:face_masker_base;
export import inference_session;

export namespace ffc::face_masker {
class FaceMaskerBase : public ai::InferenceSession {
public:
    explicit FaceMaskerBase(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~FaceMaskerBase() override = default;
};
} // namespace ffc::face_masker