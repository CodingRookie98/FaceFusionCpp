/**
 ******************************************************************************
 * @file           : face_analyser.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-3
 ******************************************************************************
 */

#ifndef FACEFUSIONCPP_SRC_FACE_ANALYSER_H_
#define FACEFUSIONCPP_SRC_FACE_ANALYSER_H_

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include "config.h"
#include "logger.h"
#include "face_store.h"
#include "face.h"
#include "face_detectors.h"
#include "face_landmarkers.h"
#include "face_recognizers.h"
#include "face_classifiers.h"


class FaceAnalyser {
public:
    explicit FaceAnalyser(const std::shared_ptr<Ort::Env> &env,
                          const std::shared_ptr<const Ffc::Config> &config);
    ~FaceAnalyser() = default;

    Face getAverageFace(const std::vector<cv::Mat> &visionFrames);

    std::vector<Face> getManyFaces(const cv::Mat &visionFrame);

    Face getOneFace(const cv::Mat &visionFrame, const unsigned int &position = 0);

    std::vector<Face> findSimilarFaces(const std::vector<Face> &referenceFaces,
                                       const cv::Mat &targetVisionFrame,
                                       const float &faceDistance);

    static bool compareFace(const Face &face, const Face &referenceFace, const float &faceDistance);

    static float calculateFaceDistance(const Face &face1, const Face &face2);

private:
    std::shared_ptr<Ort::Env> m_env;
    std::mutex m_mutex;
    const std::shared_ptr<const Ffc::Config> m_config;
    std::shared_ptr<Ffc::Logger> m_logger = Ffc::Logger::getInstance();
    std::shared_ptr<FaceStore> m_faceStore = FaceStore::getInstance();
    FaceDetectors m_faceDetectors;
    FaceLandmarkers m_faceLandmarkers;
    FaceRecognizers m_faceRecognizers;
    FaceClassifiers m_faceClassifiers;

    std::vector<Face> createFaces(const cv::Mat &visionFrame,
                                  const std::vector<Face::BBox> &bBoxes,
                                  const std::vector<Face::Landmark> &landmarks5,
                                  const std::vector<Face::Score> &scores, const double &detectedAngle);
    Face::Landmark expandFaceLandmark68By5(const Face::Landmark &inputLandmark5);

    std::array<Face::Embedding, 2>
    calculateEmbedding(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5By68);

    std::tuple<Face::Gender, Face::Age, Face::Race>
    classifyFace(const cv::Mat &visionFrame, const Face::Landmark &faceLandmarks5);
};

#endif // FACEFUSIONCPP_SRC_FACE_ANALYSER_H_
