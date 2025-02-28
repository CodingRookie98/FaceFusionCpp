/**
 ******************************************************************************
 * @file           : face_detector_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_detector_hub:face_detector_base;
export import face;
export import inference_session;

export namespace ffc::faceDetector {
class FaceDetectorBase : public InferenceSession {
public:
    explicit FaceDetectorBase(const std::shared_ptr<Ort::Env>& env = nullptr);
    ~FaceDetectorBase() override = default;

    struct Result {
        std::vector<Face::BBox> bboxes;
        std::vector<Face::Landmarks> landmarks;
        std::vector<float> scores;
    };

    virtual Result DetectFaces(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize,
                               const float& detectorScore) = 0;
    Result DetectRotatedFaces(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize,
                              const double& angle, const float& detectorScore = 0.5);
};
} // namespace ffc::faceDetector