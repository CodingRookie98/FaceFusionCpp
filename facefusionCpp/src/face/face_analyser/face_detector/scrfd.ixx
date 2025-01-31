/**
 ******************************************************************************
 * @file           : face_detector_scrfd.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-16
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_detector_hub:scrfd;
import :face_detector_base;

namespace ffc::faceDetector {

export class Scrfd final : public FaceDetectorBase {
public:
    explicit Scrfd(const std::shared_ptr<Ort::Env> &env);
    ~Scrfd() override = default;

    Result detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                       const float &detectorScore) override;

    void loadModel(const std::string &modelPath, const Options &options) override;

private:
    static std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize);
    int m_inputHeight{0};
    int m_inputWidth{0};
    const std::vector<int> m_featureStrides{8, 16, 32};
    const int m_featureMapChannel = 3;
    const int m_anchorTotal = 2;
};
} // namespace ffc::faceDetector