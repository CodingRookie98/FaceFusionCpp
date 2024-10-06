/**
 ******************************************************************************
 * @file           : face_classifier_base.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_BASE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_BASE_H_

#include "inference_session.h"
#include "face.h"

class FaceClassifierBase {
public:
    FaceClassifierBase(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    virtual ~FaceClassifierBase() = default;
    class Result {
    public:
        Face::Race race;
        Face::Gender gender;
        Face::Age age;
    };
    virtual Result classify(const cv::Mat &image, const Face::Landmark &faceLandmark5) = 0;

protected:
    Ffc::InferenceSession m_inferenceSession;
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_BASE_H_
