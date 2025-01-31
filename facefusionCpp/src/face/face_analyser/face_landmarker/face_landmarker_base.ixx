/**
 ******************************************************************************
 * @file           : face_landmarker_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_landmarker_hub:face_landmarker_base;
export import inference_session;

export namespace ffc::faceLandmarker {
class FaceLandmarkerBase : public InferenceSession {
public:
    explicit FaceLandmarkerBase(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceLandmarkerBase() override = default;

protected:
    static cv::Mat conditionalOptimizeContrast(const cv::Mat &visionFrame);
};
} // namespace ffc::faceLandmarker