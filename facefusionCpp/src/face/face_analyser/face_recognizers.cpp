/**
 ******************************************************************************
 * @file           : face_recognizers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "face_recognizers.h"
#include "model_manager.h"
#include "fr_arc_w_600_k_r_50.h"

FaceRecognizers::FaceRecognizers(const std::shared_ptr<Ort::Env> &env) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceRecognizers");
    } else {
        m_env = env;
    }
}

void FaceRecognizers::createRecognizer(const FaceRecognizers::FaceRecognizerType &type) {
    if (m_recognizers.contains(type)) {
        if (m_recognizers[type] != nullptr) {
            return;
        }
    }

    static std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    FaceRecognizerBase *recognizer = nullptr;
    if (type == FaceRecognizerType::Arc_w600k_r50) {
        recognizer = new FRArcW600kR50(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_recognizer_arcface_w600k_r50));
    }
    
    if (recognizer != nullptr) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_recognizers[type] = recognizer;
    }
}

std::array<Face::Embedding, 2> FaceRecognizers::recognize(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5, const FaceRecognizers::FaceRecognizerType &type) {
    if (!m_recognizers.contains(type)) {
        createRecognizer(type);
    }
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_recognizers[type]->recognize(visionFrame, faceLandmark5);
}

FaceRecognizers::~FaceRecognizers() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto &recognizer: m_recognizers) {
            delete recognizer.second;
    }
    m_recognizers.clear();
}
