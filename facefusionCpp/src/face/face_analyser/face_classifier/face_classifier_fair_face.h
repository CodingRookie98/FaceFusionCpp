/**
 ******************************************************************************
 * @file           : face_classifier_fair_face.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-15
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_FAIR_FACE_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_FAIR_FACE_H_

#include "face_classifier_base.h"
#include "face_helper.h"

class FaceClassifierFairFace : public FaceClassifierBase {
public:
    FaceClassifierFairFace(const std::shared_ptr<Ort::Env> &env, const std::string &modelPath);
    ~FaceClassifierFairFace() override = default;

    Result classify(const cv::Mat &image, const Face::Landmark &faceLandmark5) override;
private:
    FaceHelper::WarpTemplateType m_WarpTemplateType = FaceHelper::WarpTemplateType::Arcface_112_v2;
    cv::Size m_size = cv::Size(224, 224);
    std::array<float, 3> m_mean = {0.485, 0.456, 0.406};
    std::array<float, 3> m_standardDeviation = {0.229, 0.224, 0.225};
    int m_inputWidth;
    int m_inputHeight;
    
    std::vector<float> getInputImageData(const cv::Mat &image, const Face::Landmark &faceLandmark5);
    Face::Gender categorizeGender(const int64_t &genderId);
    Face::Age categorizeAge(const int64_t &ageId);
    Face::Race categorizeRace(const int64_t &raceId);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIER_FACE_CLASSIFIER_FAIR_FACE_H_
