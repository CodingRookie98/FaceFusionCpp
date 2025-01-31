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
    explicit FaceDetectorHub(const std::shared_ptr<Ort::Env> &env = nullptr,
                             const InferenceSession::Options &ISOptions = InferenceSession::Options{});
    ~FaceDetectorHub();

    enum class Type {
        Retina,
        Scrfd,
        Yolo,
    };

    struct Options {
        cv::Size faceDetectorSize{640, 640};
        std::unordered_set<Type> types{Type::Yolo};
        double angle{0.0};
        float minScore{0.5};
    };
    std::vector<FaceDetectorBase::Result>
    detect(const cv::Mat &image, const Options &options = Options());

private:
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<Type, FaceDetectorBase *> m_faceDetectors;
    std::shared_mutex m_sharedMutex;
    InferenceSession::Options m_ISOptions;

    FaceDetectorBase *getDetector(const Type &type);
};

} // namespace ffc::faceDetector