/**
 ******************************************************************************
 * @file           : face_detectors.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTORS_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTORS_H_

#include <shared_mutex>
#include "face_detector_base.h"

class FaceDetectors {
public:
    explicit FaceDetectors(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceDetectors();

    enum FaceDetectorType {
        Many,
        Retina,
        Scrfd,
        Yolo
    };
    std::vector<FaceDetectorBase::Result>
    detect(const cv::Mat &image, const cv::Size &faceDetectorSize,
           const FaceDetectorType &type = FaceDetectorType::Many,
           const double &angle = 0.0, const float &detectorScore = 0.5);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::unordered_map<FaceDetectorType, FaceDetectorBase *> m_faceDetectors;
    std::shared_mutex m_mutex;

    void createDetector(const FaceDetectorType &type);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTORS_H_
