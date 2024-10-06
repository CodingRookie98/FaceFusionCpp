/**
 ******************************************************************************
 * @file           : face_classifiers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "model_manager.h"
#include "face_classifiers.h"

FaceClassifiers::FaceClassifiers(const std::shared_ptr<Ort::Env> &env) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectors");
    } else {
        m_env = env;
    }
}

FaceClassifiers::~FaceClassifiers() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto &it : m_classifiers) {
        delete it.second;
    }
    m_classifiers.clear();
}

FaceClassifierBase::Result
FaceClassifiers::classify(const cv::Mat &image, const Face::Landmark &faceLandmark5,
                           const FaceClassifiers::FaceClassifierType &type) {
    createFaceClassfier(type);
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_classifiers[type]->classify(image, faceLandmark5);
}

void FaceClassifiers::createFaceClassfier(FaceClassifiers::FaceClassifierType type) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    if (m_classifiers.contains(type)) {
        if (m_classifiers[type] != nullptr) {
            return;
        }
    }

    static std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    FaceClassifierBase *classifier = nullptr;
    if (type == FairFace) {
        classifier = new FaceClassifierFairFace(m_env, modelManager->getModelPath(Ffc::ModelManager::FairFace));
    }

    if (classifier != nullptr) {
        m_classifiers[type] = classifier;
    }
}
