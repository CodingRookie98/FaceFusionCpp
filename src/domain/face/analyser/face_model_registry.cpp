module;
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <vector>

module domain.face.model_registry;

import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import foundation.ai.inference_session;

namespace domain::face {

static std::unique_ptr<FaceModelRegistry> s_instance;
static std::mutex s_instance_mutex;

FaceModelRegistry& FaceModelRegistry::get_instance() {
    std::lock_guard<std::mutex> lock(s_instance_mutex);
    if (!s_instance) { s_instance.reset(new FaceModelRegistry()); }
    return *s_instance;
}

void FaceModelRegistry::set_instance_for_testing(std::unique_ptr<FaceModelRegistry> instance) {
    std::lock_guard<std::mutex> lock(s_instance_mutex);
    s_instance = std::move(instance);
}

void FaceModelRegistry::reset_instance() {
    std::lock_guard<std::mutex> lock(s_instance_mutex);
    s_instance.reset();
}

std::string FaceModelRegistry::generate_key(
    int type, const std::string& path, const foundation::ai::inference_session::Options& options) {
    std::stringstream ss;
    ss << type << "|" << path << "|EP:";

    // Sort providers to ensure consistent key
    std::vector<int> providers;
    for (auto ep : options.execution_providers) { providers.push_back(static_cast<int>(ep)); }
    std::sort(providers.begin(), providers.end());
    for (int p : providers) ss << p << ",";

    ss << "|Dev:" << options.execution_device_id;
    ss << "|TRT:" << options.trt_max_workspace_size << "," << options.enable_tensorrt_embed_engine
       << "," << options.enable_tensorrt_cache;

    return ss.str();
}

std::shared_ptr<detector::IFaceDetector> FaceModelRegistry::get_detector(
    detector::DetectorType type, const std::string& path,
    const foundation::ai::inference_session::Options& options) {
    if (path.empty()) return nullptr;

    std::string key = generate_key(static_cast<int>(type), path, options);
    std::lock_guard lock(m_mutex);

    if (auto it = m_detectors.find(key); it != m_detectors.end()) { return it->second; }

    auto instance = detector::FaceDetectorFactory::create(type);
    if (instance) {
        instance->load_model(path, options);
        auto shared_instance = std::shared_ptr<detector::IFaceDetector>(std::move(instance));
        m_detectors[key] = shared_instance;
        return shared_instance;
    }
    return nullptr;
}

std::shared_ptr<landmarker::IFaceLandmarker> FaceModelRegistry::get_landmarker(
    landmarker::LandmarkerType type, const std::string& path,
    const foundation::ai::inference_session::Options& options) {
    if (path.empty()) return nullptr;

    std::string key = generate_key(static_cast<int>(type), path, options);
    std::lock_guard lock(m_mutex);

    if (auto it = m_landmarkers.find(key); it != m_landmarkers.end()) { return it->second; }

    auto instance = landmarker::create_landmarker(type);
    if (instance) {
        instance->load_model(path, options);
        auto shared_instance = std::shared_ptr<landmarker::IFaceLandmarker>(std::move(instance));
        m_landmarkers[key] = shared_instance;
        return shared_instance;
    }
    return nullptr;
}

std::shared_ptr<recognizer::FaceRecognizer> FaceModelRegistry::get_recognizer(
    recognizer::FaceRecognizerType type, const std::string& path,
    const foundation::ai::inference_session::Options& options) {
    if (path.empty()) return nullptr;

    std::string key = generate_key(static_cast<int>(type), path, options);
    std::lock_guard lock(m_mutex);

    if (auto it = m_recognizers.find(key); it != m_recognizers.end()) { return it->second; }

    auto instance = recognizer::create_face_recognizer(type);
    if (instance) {
        instance->load_model(path, options);
        auto shared_instance = std::shared_ptr<recognizer::FaceRecognizer>(std::move(instance));
        m_recognizers[key] = shared_instance;
        return shared_instance;
    }
    return nullptr;
}

std::shared_ptr<classifier::IFaceClassifier> FaceModelRegistry::get_classifier(
    classifier::ClassifierType type, const std::string& path,
    const foundation::ai::inference_session::Options& options) {
    if (path.empty()) return nullptr;

    std::string key = generate_key(static_cast<int>(type), path, options);
    std::lock_guard lock(m_mutex);

    if (auto it = m_classifiers.find(key); it != m_classifiers.end()) { return it->second; }

    auto instance = classifier::create_classifier(type);
    if (instance) {
        instance->load_model(path, options);
        auto shared_instance = std::shared_ptr<classifier::IFaceClassifier>(std::move(instance));
        m_classifiers[key] = shared_instance;
        return shared_instance;
    }
    return nullptr;
}

void FaceModelRegistry::clear() {
    std::lock_guard lock(m_mutex);
    m_detectors.clear();
    m_landmarkers.clear();
    m_recognizers.clear();
    m_classifiers.clear();
}

} // namespace domain::face
