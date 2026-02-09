module;
#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

export module domain.face.model_registry;

import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import foundation.ai.inference_session;

export namespace domain::face {

/**
 * @brief Registry for sharing face model instances.
 * @details This class ensures that model components (detectors, landmarkers, etc.)
 *          are shared across different FaceAnalyser instances if they use the same
 *          model path and inference options.
 */
class FaceModelRegistry {
public:
    /**
     * @brief Get the singleton instance of the registry.
     */
    static std::shared_ptr<FaceModelRegistry> get_instance();

    // Testing support
    static void set_instance_for_testing(std::unique_ptr<FaceModelRegistry> instance);
    static void reset_instance();

    /**
     * @brief Get a shared face detector.
     */
    std::shared_ptr<detector::IFaceDetector> get_detector(
        detector::DetectorType type, const std::string& path,
        const foundation::ai::inference_session::Options& options);

    /**
     * @brief Get a shared face landmarker.
     */
    std::shared_ptr<landmarker::IFaceLandmarker> get_landmarker(
        landmarker::LandmarkerType type, const std::string& path,
        const foundation::ai::inference_session::Options& options);

    /**
     * @brief Get a shared face recognizer.
     */
    std::shared_ptr<recognizer::FaceRecognizer> get_recognizer(
        recognizer::FaceRecognizerType type, const std::string& path,
        const foundation::ai::inference_session::Options& options);

    /**
     * @brief Get a shared face classifier.
     */
    std::shared_ptr<classifier::IFaceClassifier> get_classifier(
        classifier::ClassifierType type, const std::string& path,
        const foundation::ai::inference_session::Options& options);

    /**
     * @brief Clear all cached model instances.
     */
    void clear();

    // Destructor must be public for std::unique_ptr
    ~FaceModelRegistry() = default;

    FaceModelRegistry(const FaceModelRegistry&) = delete;
    FaceModelRegistry& operator=(const FaceModelRegistry&) = delete;

    FaceModelRegistry(FaceModelRegistry&&) = delete;
    FaceModelRegistry& operator=(FaceModelRegistry&&) = delete;

    FaceModelRegistry() = default;

private:
    std::mutex m_mutex;

    // We use a simple string key: "TypeID:Path:OptionsHash"
    // Since Options hashing is complex, we might just use a combined check or a simpler key for
    // now. For this implementation, we'll use TypeID + Path + some identifier for Options if
    // needed. Actually, given the scale, a simple map is enough.

    std::unordered_map<std::string, std::shared_ptr<detector::IFaceDetector>> m_detectors;
    std::unordered_map<std::string, std::shared_ptr<landmarker::IFaceLandmarker>> m_landmarkers;
    std::unordered_map<std::string, std::shared_ptr<recognizer::FaceRecognizer>> m_recognizers;
    std::unordered_map<std::string, std::shared_ptr<classifier::IFaceClassifier>> m_classifiers;

    std::string generate_key(int type, const std::string& path,
                             const foundation::ai::inference_session::Options& options);
};

} // namespace domain::face
