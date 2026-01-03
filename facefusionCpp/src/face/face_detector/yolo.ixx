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

namespace ffc::face_detector {

export class Yolo final : public FaceDetectorBase {
public:
    explicit Yolo(const std::shared_ptr<Ort::Env>& env);
    ~Yolo() override = default;

    Result DetectFaces(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize,
                       const float& scoreThreshold) override;
    void load_model(const std::string& modelPath, const Options& options) override;

    static inline std::vector<cv::Size> GetSupportSizes() {
        return {{640, 640}};
    }

private:
    static std::tuple<std::vector<float>, float, float> preProcess(const cv::Mat& visionFrame, const cv::Size& faceDetectorSize);
    int input_height_{0};
    int input_width_{0};
};

} // namespace ffc::face_detector
