/**
 ******************************************************************************
 * @file           : face_recognizer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_recognizer_hub:face_recognizer_base;
export import face;
export import inference_session;

namespace ffc::face_recognizer {
export class FaceRecognizerBase : public ai::InferenceSession {
public:
    explicit FaceRecognizerBase(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~FaceRecognizerBase() override = default;

    virtual std::array<std::vector<float>, 2> recognize(const cv::Mat& visionFrame, const Face::Landmarks& faceLandmark5) = 0;
};
} // namespace ffc::face_recognizer