/**
 ******************************************************************************
 * @file           : face_landmarker_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_BASE_H_

#include "inference_session.h"
#include <opencv2/opencv.hpp>

class FaceLandmarkerBase {
public:
    FaceLandmarkerBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    virtual ~FaceLandmarkerBase() = default;

protected:
    Ffc::InferenceSession m_inferenceSession;

    static cv::Mat conditionalOptimizeContrast(const cv::Mat &visionFrame);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_LANDMARKER_FACE_LANDMARKER_BASE_H_
