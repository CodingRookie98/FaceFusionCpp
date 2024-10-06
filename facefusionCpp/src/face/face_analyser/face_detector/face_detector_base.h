/**
 ******************************************************************************
 * @file           : face_detector_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-9
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTOR_FACE_DETECTOR_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTOR_FACE_DETECTOR_BASE_H_

#include "inference_session.h"
#include "face.h"

class FaceDetectorBase {
public:
    FaceDetectorBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    virtual ~FaceDetectorBase() = default;

    class Result {
    public:
        std::vector<Face::BBox> bboxes;
        std::vector<Face::Landmark> landmarks;
        std::vector<float> scores;

        Result() = default;
    };
    
    virtual Result detectFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                               const float &detectorScore) = 0;
    Result detectRotatedFaces(const cv::Mat &visionFrame, const cv::Size &faceDetectorSize,
                       const double &angle, const float &detectorScore = 0.5);

protected:
    Ffc::InferenceSession m_inferenceSession;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_DETECTOR_FACE_DETECTOR_BASE_H_
