/**
 ******************************************************************************
 * @file           : face_detectors.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_detector_hub;
export import :face_detector_base;

export namespace ffc::faceDetector {

class FaceDetectorHub {
public:
    explicit FaceDetectorHub(const std::shared_ptr<Ort::Env>& env = nullptr,
                             const InferenceSession::Options& ISOptions = InferenceSession::Options{});
    ~FaceDetectorHub();

    enum class Type {
        Retina,
        Scrfd,
        Yolo,
    };

    struct Options {
        cv::Size face_detector_size{640, 640};
        std::unordered_set<Type> types{Type::Yolo};
        double angle{0.0};
        float min_score{0.5};
    };
    std::vector<FaceDetectorBase::Result>
    Detect(const cv::Mat& image, const Options& options = Options());

    static std::vector<cv::Size> GetSupportSizes(const Type& type);
    static std::vector<cv::Size> GetSupportCommonSizes(const std::unordered_set<Type>& types);

private:
    std::shared_ptr<Ort::Env> env_;
    std::unordered_map<Type, std::shared_ptr<FaceDetectorBase>> face_detectors_;
    std::shared_mutex shared_mutex_;
    InferenceSession::Options inference_session_options_;

    std::shared_ptr<FaceDetectorBase> GetDetector(const Type& type);
};

} // namespace ffc::faceDetector