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
#include <ranges>
#include <shared_mutex>

module face_classifier_hub;
import :fair_face;
import model_manager;

namespace ffc::face_classifier {

using namespace ai::model_manager;

FaceClassifierHub::FaceClassifierHub(const std::shared_ptr<Ort::Env>& env,
                                     const ai::InferenceSession::Options& options) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectorHub");
    } else {
        m_env = env;
    }
    m_ISOptions = options;
}

FaceClassifierHub::~FaceClassifierHub() {
    std::unique_lock lock(m_sharedMutex);
    for (const auto& val : m_classifiers | std::views::values) { delete val; }
    m_classifiers.clear();
}

FaceClassifierBase::Result FaceClassifierHub::classify(const cv::Mat& image,
                                                       const Face::Landmarks& faceLandmark5,
                                                       const Type& type) {
    return get_face_classifier(type)->classify(image, faceLandmark5);
}

FaceClassifierBase* FaceClassifierHub::get_face_classifier(const Type& type) {
    std::unique_lock lock(m_sharedMutex);
    if (m_classifiers.contains(type)) {
        if (m_classifiers[type] != nullptr) { return m_classifiers[type]; }
    }

    static std::shared_ptr<ModelManager> modelManager = ModelManager::get_instance();
    if (type == Type::FairFace) {
        FaceClassifierBase* classifier{nullptr};
        classifier = new FairFace(m_env);
        if (classifier == nullptr) { return nullptr; }
        classifier->load_model(modelManager->get_model_path(Model::FairFace), m_ISOptions);
        m_classifiers[type] = classifier;
    }

    if (m_classifiers.contains(type) && m_classifiers[type] != nullptr) {
        return m_classifiers[type];
    }
    return nullptr;
}
} // namespace ffc::face_classifier