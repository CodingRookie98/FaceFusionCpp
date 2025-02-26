/**
 ******************************************************************************
 * @file           : face_detector_retina.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-16
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_detector_hub:retina;
import :face_detector_base;

namespace ffc::faceDetector {
export class Retina final : public FaceDetectorBase {
public:
    explicit Retina(const std::shared_ptr<Ort::Env>& env);
    ~Retina() override = default;

    Result detectFaces(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize,
                       const float& scoreThreshold) override;

    void LoadModel(const std::string& modelPath, const Options& options) override;

    static inline std::vector<cv::Size> GetSupportSizes() {
        return {{160, 160}, {320, 320}, {480, 480}, {512, 512}, {640, 640}};
    }

private:
    static std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize);
    int m_inputHeight{0};
    int m_inputWidth{0};
    const std::vector<int> m_featureStrides{8, 16, 32};
    const int m_featureMapChannel{3};
    const int m_anchorTotal{2};
};
} // namespace ffc::faceDetector