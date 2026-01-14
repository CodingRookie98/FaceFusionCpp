module;
#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <opencv2/core.hpp>

export module domain.face.analyser;

import domain.face;
import domain.face.detector;
import domain.face.landmarker;
import domain.face.recognizer;
import domain.face.classifier;
import domain.face.selector;
import domain.face.store;
import domain.common;
import foundation.ai.inference_session;

export namespace domain::face::analyser {

struct ModelPaths {
    std::string face_detector_yolo;
    std::string face_detector_scrfd;
    std::string face_detector_retina;
    std::string face_landmarker_2dfan;
    std::string face_landmarker_peppawutz;
    std::string face_landmarker_68by5;
    std::string face_recognizer_arcface;
    std::string face_classifier_fairface;
};

struct FaceDetectorOptions {
    detector::DetectorType type = detector::DetectorType::Yolo;
    float min_score = 0.5f;
    float iou_threshold = 0.4f;
};

struct FaceLandmarkerOptions {
    landmarker::LandmarkerType type = landmarker::LandmarkerType::_2DFAN;
    float min_score = 0.5f;
};

struct Options {
    ModelPaths model_paths;

    FaceDetectorOptions face_detector_options;
    FaceLandmarkerOptions face_landmarker_options;

    recognizer::FaceRecognizerType face_recognizer_type =
        recognizer::FaceRecognizerType::ArcFace_w600k_r50;
    classifier::ClassifierType face_classifier_type = classifier::ClassifierType::FairFace;

    selector::Options face_selector_options;
    foundation::ai::inference_session::Options inference_session_options;
};

class FaceAnalyser {
public:
    explicit FaceAnalyser(const Options& options);

    FaceAnalyser(const Options& options, std::unique_ptr<detector::IFaceDetector> detector,
                 std::unique_ptr<landmarker::IFaceLandmarker> landmarker,
                 std::unique_ptr<recognizer::FaceRecognizer> recognizer,
                 std::unique_ptr<classifier::IFaceClassifier> classifier,
                 std::unique_ptr<store::FaceStore> store = nullptr);

    ~FaceAnalyser();

    FaceAnalyser(const FaceAnalyser&) = delete;
    FaceAnalyser& operator=(const FaceAnalyser&) = delete;

    std::vector<Face> get_many_faces(const cv::Mat& vision_frame);
    Face get_one_face(const cv::Mat& vision_frame, unsigned int position = 0);
    Face get_average_face(const std::vector<cv::Mat>& vision_frames);
    Face get_average_face(const std::vector<Face>& faces);

    std::vector<Face> find_similar_faces(const std::vector<Face>& reference_faces,
                                         const cv::Mat& target_vision_frame, float face_distance);

    static bool compare_face(const Face& face, const Face& reference_face, float face_distance);
    static float calculate_face_distance(const Face& face1, const Face& face2);

private:
    std::vector<Face> create_faces(const cv::Mat& vision_frame,
                                   const std::vector<detector::DetectionResult>& detection_results,
                                   double detected_angle);

    std::pair<types::Embedding, types::Embedding> calculate_embedding(
        const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5);

    std::tuple<domain::common::types::Gender, domain::common::types::AgeRange,
               domain::common::types::Race>
    classify_face(const cv::Mat& vision_frame, const types::Landmarks& face_landmark_5);

    Options m_options;
    std::unique_ptr<detector::IFaceDetector> m_detector;
    std::unique_ptr<landmarker::IFaceLandmarker> m_landmarker;
    std::unique_ptr<recognizer::FaceRecognizer> m_recognizer;
    std::unique_ptr<classifier::IFaceClassifier> m_classifier;
    std::unique_ptr<store::FaceStore> m_face_store;
};

} // namespace domain::face::analyser
