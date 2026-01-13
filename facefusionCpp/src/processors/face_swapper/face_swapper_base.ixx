/**
 ******************************************************************************
 * @file           : face_swapper_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <string>
#include <memory>

export module face_swapper:face_swapper_base;
import processor_base;
import face_masker_hub;

export namespace ffc::faceSwapper {

using namespace face_masker;

class FaceSwapperBase : public ProcessorBase {
public:
    explicit FaceSwapperBase() = default;
    ~FaceSwapperBase() override = default;

    [[nodiscard]] std::string get_processor_name() const override = 0;

    void set_face_masker_hub(const std::shared_ptr<FaceMaskerHub>& face_masker_hub) {
        m_face_masker_hub = face_masker_hub;
    }

    [[nodiscard]] bool has_face_masker_hub() const {
        if (m_face_masker_hub == nullptr) { return false; }
        return true;
    }

protected:
    std::shared_ptr<FaceMaskerHub> m_face_masker_hub;
};

}; // namespace ffc::faceSwapper
