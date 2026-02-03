module;
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>

/**
 * @file face_analyser.ixx
 * @brief High-level face analysis and processing module
 * @author CodingRookie
 * @date 2026-01-27
 */
export module domain.face.analyser;

import domain.face;
import domain.face.model_registry;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.selector;
import domain.face.store;
import domain.common;
import foundation.ai.inference_session;

export namespace domain::face::analyser {

/**
 * @brief Paths for all face models
 */
struct ModelPaths {
    std::string face_detector_yolo;        ///< Path to YOLO face detector model
    std::string face_detector_scrfd;       ///< Path to SCRFD face detector model
    std::string face_detector_retina;      ///< Path to RetinaFace face detector model
    std::string face_landmarker_2dfan;     ///< Path to 2DFAN face landmarker model
    std::string face_landmarker_peppawutz; ///< Path to Peppawutz face landmarker model
    std::string face_landmarker_68by5;     ///< Path to 68-from-5 landmark model
    std::string face_recognizer_arcface;   ///< Path to ArcFace face recognizer model
    std::string face_classifier_fairface;  ///< Path to FairFace face classifier model
};

/**
 * @brief Options for face detection
 */
struct FaceDetectorOptions {
    detector::DetectorType type = detector::DetectorType::Yolo; ///< Preferred detector type
    float min_score = 0.5f;                                     ///< Minimum confidence score
    float iou_threshold = 0.4f;                                 ///< NMS IOU threshold
};

/**
 * @brief Options for face landmarking
 */
struct FaceLandmarkerOptions {
    landmarker::LandmarkerType type =
        landmarker::LandmarkerType::_2DFAN; ///< Preferred landmarker type
    float min_score = 0.5f;                 ///< Minimum confidence score
};

/**
 * @brief Configuration options for FaceAnalyser
 */
struct Options {
    ModelPaths model_paths;                        ///< Paths to required model files
    FaceDetectorOptions face_detector_options;     ///< Configuration for detection
    FaceLandmarkerOptions face_landmarker_options; ///< Configuration for landmarking
    recognizer::FaceRecognizerType face_recognizer_type =
        recognizer::FaceRecognizerType::ArcFace_w600k_r50; ///< Preferred recognizer model
    classifier::ClassifierType face_classifier_type =
        classifier::ClassifierType::FairFace; ///< Preferred classifier model
    selector::Options face_selector_options;  ///< Configuration for face selection
    foundation::ai::inference_session::Options inference_session_options; ///< ONNX runtime settings
};

/**
 * @brief Face Analysis Types (Bitmask)
 */
enum class FaceAnalysisType : unsigned int {
    None = 0,                                          ///< Perform no analysis
    Detection = 1 << 0,                                ///< Perform face detection
    Landmark = 1 << 1,                                 ///< Perform landmark detection
    Embedding = 1 << 2,                                ///< Extract face embedding
    GenderAge = 1 << 3,                                ///< Predict gender and age
    All = Detection | Landmark | Embedding | GenderAge ///< Perform all analysis steps
};

/**
 * @brief Bitwise OR operator for FaceAnalysisType
 */
constexpr FaceAnalysisType operator|(FaceAnalysisType lhs, FaceAnalysisType rhs) {
    return static_cast<FaceAnalysisType>(static_cast<unsigned int>(lhs)
                                         | static_cast<unsigned int>(rhs));
}

/**
 * @brief Bitwise AND operator for FaceAnalysisType
 */
constexpr FaceAnalysisType operator&(FaceAnalysisType lhs, FaceAnalysisType rhs) {
    return static_cast<FaceAnalysisType>(static_cast<unsigned int>(lhs)
                                         & static_cast<unsigned int>(rhs));
}

/**
 * @brief Helper to check if a specific flag is set in a FaceAnalysisType bitmask
 */
constexpr bool has_flag(FaceAnalysisType value, FaceAnalysisType flag) {
    return (value & flag) == flag;
}

/**
 * @brief FaceAnalyser orchestrates face detection, recognition, and analysis
 * @details This class manages the lifecycle of various face models and provides high-level APIs
 *          to extract face information from images.
 */
class FaceAnalyser {
public:
    /**
     * @brief Construct a FaceAnalyser with specified options
     * @param options Configuration for the analyser
     */
    explicit FaceAnalyser(const Options& options);

    /**
     * @brief Construct a FaceAnalyser with pre-initialized models
     * @param options Configuration for the analyser
     * @param detector Face detector instance
     * @param landmarker Face landmarker instance
     * @param recognizer Face recognizer instance
     * @param classifier Face classifier instance
     * @param store Optional face store for caching results
     */
    FaceAnalyser(const Options& options, std::shared_ptr<detector::IFaceDetector> detector,
                 std::shared_ptr<landmarker::IFaceLandmarker> landmarker,
                 std::shared_ptr<recognizer::FaceRecognizer> recognizer,
                 std::shared_ptr<classifier::IFaceClassifier> classifier,
                 std::shared_ptr<store::FaceStore> store = nullptr);

    ~FaceAnalyser();

    FaceAnalyser(const FaceAnalyser&) = delete;
    FaceAnalyser& operator=(const FaceAnalyser&) = delete;

    /**
     * @brief Update options and reload models if necessary
     * @param options New configuration options
     */
    void update_options(const Options& options);

    /**
     * @brief Detect and analyze multiple faces in a frame
     * @param vision_frame Input image frame
     * @param type Bitmask determining which analysis steps to perform
     * @return Vector of detected Face objects
     */
    std::vector<Face> get_many_faces(const cv::Mat& vision_frame,
                                     FaceAnalysisType type = FaceAnalysisType::All);

    /**
     * @brief Detect and get a single face (based on selection strategy)
     * @param vision_frame Input image frame
     * @param position Face position index (if applicable)
     * @param type Bitmask determining which analysis steps to perform
     * @return Detected Face object, or empty Face if none found
     */
    Face get_one_face(const cv::Mat& vision_frame, unsigned int position = 0,
                      FaceAnalysisType type = FaceAnalysisType::All);

    /**
     * @brief Calculate the average face embedding from multiple frames
     * @param vision_frames List of image frames containing the same face
     * @return A virtual Face object representing the average face
     */
    Face get_average_face(const std::vector<cv::Mat>& vision_frames);

    /**
     * @brief Calculate the average face from a list of pre-detected faces
     * @param faces List of Face objects
     * @return A virtual Face object representing the average face
     */
    Face get_average_face(const std::vector<Face>& faces);

    /**
     * @brief Find faces similar to reference faces in a target frame
     * @param reference_faces List of known faces to search for
     * @param target_vision_frame Frame to search in
     * @param face_distance Similarity threshold (distance)
     * @return Vector of matching Face objects found in the target frame
     */
    std::vector<Face> find_similar_faces(const std::vector<Face>& reference_faces,
                                         const cv::Mat& target_vision_frame, float face_distance);

    /**
     * @brief Compare two faces for similarity
     * @param face Detected face
     * @param reference_face Reference face
     * @param face_distance Max distance threshold for match
     * @return True if faces match
     */
    static bool compare_face(const Face& face, const Face& reference_face, float face_distance);

    /**
     * @brief Calculate Euclidean distance between face embeddings
     * @param face1 First face
     * @param face2 Second face
     * @return Distance value
     */
    static float calculate_face_distance(const Face& face1, const Face& face2);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace domain::face::analyser
