/**
 ******************************************************************************
 * @file           : face_recognizer_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FACE_RECOGNIZER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FACE_RECOGNIZER_BASE_H_

#include <opencv2/opencv.hpp>
#include "inference_session.h"
#include "face.h"

class FaceRecognizerBase {
public:
    FaceRecognizerBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    virtual ~FaceRecognizerBase() = default;

    virtual std::array<std::vector<float>, 2> recognize(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5) = 0;

protected:
    Ffc::InferenceSession m_inferenceSession;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_RECOGNIZER_FACE_RECOGNIZER_BASE_H_
