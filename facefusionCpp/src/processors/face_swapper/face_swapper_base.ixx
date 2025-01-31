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

using namespace faceMasker;

class FaceSwapperBase : public ProcessorBase {
public:
    explicit FaceSwapperBase() = default;
    ~FaceSwapperBase() override = default;

    [[nodiscard]] std::string getProcessorName() const override = 0;

    void setFaceMaskerHub(const std::shared_ptr<FaceMaskerHub> &_faceMaskerHub) {
        m_faceMaskerHub = _faceMaskerHub;
    }

    [[nodiscard]] bool hasFaceMaskerHub() const {
        if (m_faceMaskerHub == nullptr) {
            return false;
        }
        return true;
    }

protected:
    std::shared_ptr<FaceMaskerHub> m_faceMaskerHub;
};

}; // namespace ffc::faceSwapper
