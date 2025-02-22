/**
 ******************************************************************************
 * @file           : face_classifiers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <memory>
#include <shared_mutex>

module face_classifier_hub;
import :fair_face;
import model_manager;

namespace ffc::faceClassifier {

FaceClassifierHub::FaceClassifierHub(const std::shared_ptr<Ort::Env> &env, const ffc::InferenceSession::Options &options) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectorHub");
    } else {
        m_env = env;
    }
    m_ISOptions = options;
}

FaceClassifierHub::~FaceClassifierHub() {
    std::unique_lock lock(m_sharedMutex);
    for (auto &it : m_classifiers) {
        delete it.second;
    }
    m_classifiers.clear();
}

FaceClassifierBase::Result
FaceClassifierHub::classify(const cv::Mat &image, const Face::Landmarks &faceLandmark5,
                            const FaceClassifierHub::Type &type) {
    return getFaceClassifier(type)->classify(image, faceLandmark5);
}

FaceClassifierBase *FaceClassifierHub::getFaceClassifier(FaceClassifierHub::Type type) {
    std::unique_lock lock(m_sharedMutex);
    if (m_classifiers.contains(type)) {
        if (m_classifiers[type] != nullptr) {
            return m_classifiers[type];
        }
    }

    static std::shared_ptr<ffc::ModelManager> modelManager = ffc::ModelManager::getInstance();
    FaceClassifierBase *classifier = nullptr;
    if (type == Type::FairFace) {
        classifier = new FairFace(m_env);
        classifier->loadModel(modelManager->getModelPath(ffc::ModelManager::Model::FairFace), m_ISOptions);
    }

    if (classifier) {
        m_classifiers[type] = classifier;
    } else {
        return nullptr;
    }
    return m_classifiers[type];
}
} // namespace ffc::faceClassifier