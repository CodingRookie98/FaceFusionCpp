/**
 ******************************************************************************
 * @file           : face_classifiers.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIERS_H_
#define FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIERS_H_

#include <shared_mutex>
#include "face_classifier_fair_face.h"

class FaceClassifiers {
public:
    explicit FaceClassifiers(const std::shared_ptr<Ort::Env> &env = nullptr);
    ~FaceClassifiers();

    enum FaceClassifierType {
        FairFace,
    };
    FaceClassifierBase::Result
    classify(const cv::Mat &image,
             const Face::Landmark &faceLandmark5, const FaceClassifierType &type);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::shared_mutex m_mutex;
    std::unordered_map<FaceClassifierType, FaceClassifierBase *> m_classifiers;

    void createFaceClassfier(FaceClassifierType type);
};

#endif // FACEFUSIONCPP_FACEFUSIONCPP_SRC_FACE_FACE_ANALYSER_FACE_CLASSIFIERS_H_
