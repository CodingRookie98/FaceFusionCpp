/**
 ******************************************************************************
 * @file           : face_recognizer_hub.mxx
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <memory>
#include <shared_mutex>
#include <ranges>
#include <onnxruntime_cxx_api.h>

module face_recognizer_hub;
import :arc_w600k_r50;
import model_manager;

namespace ffc::face_recognizer {

using namespace ai::model_manager;

FaceRecognizerHub::FaceRecognizerHub(const std::shared_ptr<Ort::Env>& env,
                                     const ai::InferenceSession::Options& ISOptions) {
    m_env = env;
    m_ISOptions = ISOptions;
}

FaceRecognizerBase* FaceRecognizerHub::getRecognizer(const FaceRecognizerHub::Type& type) {
    std::unique_lock lock(m_sharedMutex);
    if (m_recognizers.contains(type)) {
        if (m_recognizers[type] != nullptr) { return m_recognizers[type]; }
    }

    static std::shared_ptr<ModelManager> modelManager = ModelManager::get_instance();
    FaceRecognizerBase* recognizer = nullptr;
    if (type == Type::Arc_w600k_r50) {
        recognizer = new ArcW600kR50(m_env);
        recognizer->load_model(
            modelManager->get_model_path(Model::Face_recognizer_arcface_w600k_r50), m_ISOptions);
    }

    if (recognizer != nullptr) { m_recognizers[type] = recognizer; }
    return m_recognizers[type];
}

std::array<Face::Embedding, 2> FaceRecognizerHub::recognize(const cv::Mat& visionFrame,
                                                            const Face::Landmarks& faceLandmark5,
                                                            const FaceRecognizerHub::Type& type) {
    return getRecognizer(type)->recognize(visionFrame, faceLandmark5);
}

FaceRecognizerHub::~FaceRecognizerHub() {
    std::unique_lock lock(m_sharedMutex);
    for (auto& val : m_recognizers | std::views::values) { delete val; }
    m_recognizers.clear();
}
} // namespace ffc::face_recognizer