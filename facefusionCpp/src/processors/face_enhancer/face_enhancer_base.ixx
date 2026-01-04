/**
 ******************************************************************************
 * @file           : face_enhancer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-20
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>

export module face_enhancer:face_enhancer_base;
import face_masker_hub;
import processor_base;

export namespace ffc::faceEnhancer {
using namespace face_masker;

class FaceEnhancerBase : public ProcessorBase {
public:
    explicit FaceEnhancerBase() = default;
    ~FaceEnhancerBase() override = default;

    std::string get_processor_name() const override = 0;

    void setFaceMaskerHub(const std::shared_ptr<FaceMaskerHub>& _faceMaskerHub) {
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

    [[nodiscard]] static cv::Mat blendFrame(const cv::Mat& targetFrame, const cv::Mat& pasteVisionFrame, ushort blend = 80);
};

} // namespace ffc::faceEnhancer
