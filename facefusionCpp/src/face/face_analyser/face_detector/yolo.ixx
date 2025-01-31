/**
 ******************************************************************************
 * @file           : face_yolov_8.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-4
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_detector_hub:yolo;
import :face_detector_base;

namespace ffc::faceDetector {

export class Yolo final : public FaceDetectorBase {
public:
    explicit Yolo(const std::shared_ptr<Ort::Env> &env);
    ~Yolo() override = default;

    Result detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                       const float &scoreThreshold) override;
    void loadModel(const std::string &modelPath, const Options &options) override;

private:
    static std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize);
    int m_inputHeight{0};
    int m_inputWidth{0};
};

} // namespace ffc::faceDetector
