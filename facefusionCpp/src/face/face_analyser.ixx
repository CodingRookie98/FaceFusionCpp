/**
 ******************************************************************************
 * @file           : face_analyser.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-3
 ******************************************************************************
 */

module;
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

export module face_analyser;

import face_store;
import face_detector_hub;
import face_landmarker_hub;
import face_recognizer_hub;
import face_classifier_hub;
import face_selector;
import face;
import inference_session;

namespace ffc {

using namespace face_detector;
using namespace face_landmarker;
using namespace face_recognizer;
using namespace face_classifier;
using namespace ffc::ai;

export class FaceAnalyser {
public:
    explicit FaceAnalyser(const std::shared_ptr<Ort::Env>& env,
                          const InferenceSession::Options& ISOptions);
    ~FaceAnalyser() = default;

    struct Options {
        FaceDetectorHub::Options faceDetectorOptions{};
        FaceLandmarkerHub::Options faceLandMarkerOptions{};
        FaceSelector::Options faceSelectorOptions{};
        FaceRecognizerHub::Type faceRecognizerType{FaceRecognizerHub::Type::Arc_w600k_r50};
    };

    // Calculate the average of embeddings
    Face GetAverageFace(const std::vector<cv::Mat>& visionFrames, const Options& options);
    // Calculate the average of embeddings
    static Face GetAverageFace(const std::vector<Face>& faces);

    std::vector<Face> GetManyFaces(const cv::Mat& visionFrame, const Options& options);

    Face GetOneFace(const cv::Mat& visionFrame, const Options& options, const unsigned int& position = 0);

    std::vector<Face> FindSimilarFaces(const std::vector<Face>& referenceFaces,
                                       const cv::Mat& targetVisionFrame,
                                       const float& faceDistance, const Options& options);

    static bool CompareFace(const Face& face, const Face& referenceFace, const float& faceDistance);

    static float CalculateFaceDistance(const Face& face1, const Face& face2);

    std::shared_ptr<FaceStore> GetFaceStore() {
        if (faceStore_ == nullptr) {
            faceStore_ = std::make_shared<FaceStore>();
        }
        return faceStore_;
    }

private:
    std::shared_ptr<Ort::Env> env_;
    FaceDetectorHub faceDetectorHub_;
    FaceLandmarkerHub faceLandMarkerHub_;
    FaceRecognizerHub faceRecognizerHub_;
    FaceClassifierHub faceClassifierHub_;
    InferenceSession::Options ISOptions_;
    std::shared_ptr<FaceStore> faceStore_;

    std::vector<Face> CreateFaces(const cv::Mat& visionFrame,
                                  const std::vector<BBox>& bBoxes,
                                  const std::vector<Face::Landmarks>& landmarks5,
                                  const std::vector<Face::Score>& scores, const double& detectedAngle,
                                  const Options& options);
    Face::Landmarks ExpandFaceLandmarks68From5(const Face::Landmarks& inputLandmark5);

    std::array<Face::Embedding, 2> CalculateEmbedding(const cv::Mat& visionFrame,
                                                      const Face::Landmarks& faceLandmark5By68,
                                                      const FaceRecognizerHub::Type& type);

    std::tuple<Gender, AgeRange, Race>
    ClassifyFace(const cv::Mat& visionFrame, const Face::Landmarks& faceLandmarks5);
};
} // namespace ffc