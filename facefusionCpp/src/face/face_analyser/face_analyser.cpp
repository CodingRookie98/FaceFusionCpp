/**
 ******************************************************************************
 * @file           : face_analyser.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-3
 ******************************************************************************
 */

#include "face_analyser.h"
#include "face_selector.h"
#include "face_swapper_helper.h"

FaceAnalyser::FaceAnalyser(const std::shared_ptr<Ort::Env> &env,
                           const std::shared_ptr<const Ffc::Config> &config) :
    m_config(config), m_env(env), m_faceDetectors(env), m_faceLandmarkers(env), m_faceRecognizers(env) {
}

Face FaceAnalyser::getAverageFace(const std::vector<cv::Mat> &visionFrames) {
    if (visionFrames.empty()) {
        return {};
    }

    std::vector<Face> faces;
    for (const auto &visionFrame : visionFrames) {
        std::vector<Face> manyFaces = this->getManyFaces(visionFrame);
        faces.insert(faces.end(), manyFaces.begin(), manyFaces.end());
    }

    if (faces.empty()) {
        return {};
    }

    Face averageFace = faces.front();
    if (faces.size() > 1) {
        std::vector<Face::Embedding> embeddings, normEmbeddings;
        for (auto &face : faces) {
            embeddings.push_back(std::move(face.m_embedding));
            normEmbeddings.push_back(std::move(face.m_normedEmbedding));
        }
        faces.clear();

        averageFace.m_embedding = FaceHelper::calcAverageEmbedding(embeddings);
        averageFace.m_normedEmbedding = FaceHelper::calcAverageEmbedding(normEmbeddings);
    }

    return averageFace;
}

Face FaceAnalyser::getOneFace(const cv::Mat &visionFrame, const unsigned int &position) {
    std::vector<Face> manyFaces = this->getManyFaces(visionFrame);
    if (!manyFaces.empty()) {
        if (position < 0 || position >= manyFaces.size()) {
            if (!manyFaces.empty()) {
                return manyFaces.back();
            } else {
                throw std::runtime_error("FaceAnalyser::getOneFace: position out of range");
            }
        } else {
            return manyFaces.at(position);
        }
    }
    return {};
}

std::vector<Face> FaceAnalyser::getManyFaces(const cv::Mat &visionFrame) {
    std::vector<Face::BBox> resultBboxes;
    std::vector<Face::Landmark> resultLandmarks5;
    std::vector<Face::Score> resultScores;

    std::vector<FaceDetectorBase::Result> detectResults;
    double detectedAngle = 0;
    // 按逆时针方向每旋转90度探测一次，探测到脸或angle=270
    for (int angle = 0; angle < 360; angle += 90) {
        detectResults = m_faceDetectors.detect(visionFrame, m_config->m_faceDetectorSize, m_config->m_faceDetectorModel, angle, m_config->m_faceDetectorScore);
        int emptyCount = 0;
        for (const auto &detectResult : detectResults) {
            if (detectResult.bboxes.empty() || detectResult.landmarks.empty() || detectResult.scores.empty()) {
                emptyCount++;
            }
        }
        if (emptyCount < detectResults.size()) {
            detectedAngle = angle;
            break;
        }
    }
    for (auto &detectResult : detectResults) {
        resultBboxes.insert(resultBboxes.end(), std::make_move_iterator(detectResult.bboxes.begin()), std::make_move_iterator(detectResult.bboxes.end()));
        resultLandmarks5.insert(resultLandmarks5.end(), std::make_move_iterator(detectResult.landmarks.begin()), std::make_move_iterator(detectResult.landmarks.end()));
        resultScores.insert(resultScores.end(), std::make_move_iterator(detectResult.scores.begin()), std::make_move_iterator(detectResult.scores.end()));
    }
    if (resultBboxes.empty() || resultLandmarks5.empty() || resultScores.empty()) {
        return {};
    }

    std::vector<Face> resultFaces = createFaces(visionFrame, resultBboxes, resultLandmarks5, resultScores, detectedAngle);

    return resultFaces;
}

Face::Landmark FaceAnalyser::expandFaceLandmark68By5(const Face::Landmark &inputLandmark5) {
    return m_faceLandmarkers.expandLandmark68By5(inputLandmark5);
}

std::vector<Face>
FaceAnalyser::createFaces(const cv::Mat &visionFrame, const std::vector<Face::BBox> &bBoxes,
                          const std::vector<Face::Landmark> &landmarks5,
                          const std::vector<Face::Score> &scores, const double &detectedAngle) {
    std::vector<Face> resultFaces;
    if (m_config->m_faceDetectorScore <= 0) {
        return resultFaces;
    }

    float iouThreshold = m_config->m_faceDetectorModel == FaceDetectors::Many ? 0.1 : 0.4;
    std::vector<int> keepIndices = FaceHelper::applyNms(bBoxes, scores, iouThreshold);
    for (const auto &index : keepIndices) {
        Face tempFace;
        tempFace.m_bBox = bBoxes.at(index);
        tempFace.m_landmark5 = landmarks5.at(index);
        tempFace.m_landmark68By5 = expandFaceLandmark68By5(tempFace.m_landmark5);
        tempFace.m_detectorScore = scores.at(index);
        
        if (m_config->m_faceLandmarkerScore > 0) {
            std::tie(tempFace.m_landmark68, tempFace.m_landmarkerScore) = m_faceLandmarkers.detectLandmark68(visionFrame, tempFace.m_bBox, detectedAngle, m_config->m_faceLandmarkerModel);

            if (tempFace.m_landmarkerScore < m_config->m_faceLandmarkerScore) {
                bool isFound = false;
                for (int angle = 90; angle < 360; angle += 90) {
                    std::tie(tempFace.m_landmark68, tempFace.m_landmarkerScore) = m_faceLandmarkers.detectLandmark68(visionFrame, tempFace.m_bBox, angle, m_config->m_faceLandmarkerModel);
                    if (tempFace.m_landmarkerScore > m_config->m_faceLandmarkerScore) {
                        tempFace.m_landMark5By68 = FaceHelper::convertFaceLandmark68To5(tempFace.m_landmark68);
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) {
                    tempFace.m_landmark68 = tempFace.m_landmark68By5;
                    tempFace.m_landMark5By68 = tempFace.m_landmark5;
                    tempFace.m_landmarkerScore = 0;
                }
            } else {
                tempFace.m_landMark5By68 = FaceHelper::convertFaceLandmark68To5(tempFace.m_landmark68);
            }
        }

        std::array<Face::Embedding, 2> embeddingAndNormedEmbedding = this->calculateEmbedding(visionFrame, tempFace.m_landMark5By68);
        tempFace.m_embedding = std::move(embeddingAndNormedEmbedding[0]);
        tempFace.m_normedEmbedding = std::move(embeddingAndNormedEmbedding[1]);

        std::tie(tempFace.m_gender, tempFace.m_age, tempFace.m_race) = this->classifyFace(visionFrame, tempFace.m_landMark5By68);

        resultFaces.emplace_back(tempFace);
    }

    if (resultFaces.empty()) {
        return {};
    }

    resultFaces = FaceSelector::filterByAge(resultFaces, m_config->m_faceSelectorAgeStart, m_config->m_faceSelectorAgeEnd);
    resultFaces = FaceSelector::filterByGender(resultFaces, m_config->m_faceSelectorGender);
    resultFaces = FaceSelector::filterByRace(resultFaces, m_config->m_faceSelectorRace);
    resultFaces = FaceSelector::sortByOrder(resultFaces, m_config->m_faceSelectorOrder);
    return resultFaces;
}

std::array<Face::Embedding, 2> FaceAnalyser::calculateEmbedding(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5By68) {
    FaceRecognizers::FaceRecognizerType faceRecognizerType = FaceSwapperHelper::getFaceRecognizerOfFaceSwapper(m_config->m_faceSwapperModel);
    return m_faceRecognizers.recognize(visionFrame, faceLandmark5By68, faceRecognizerType);
}

std::tuple<Face::Gender, Face::Age, Face::Race> FaceAnalyser::classifyFace(const cv::Mat &visionFrame, const Face::Landmark &faceLandmarks5) {
    FaceClassifierBase::Result result = m_faceClassifiers.classify(visionFrame, faceLandmarks5, FaceClassifiers::FairFace);
    return std::make_tuple(result.gender, result.age, result.race);
}

float FaceAnalyser::calculateFaceDistance(const Face &face1, const Face &face2) {
    float distance = 0.0f;
    if (!face1.m_normedEmbedding.empty() && !face2.m_normedEmbedding.empty()) {
        float dotProduct = std::inner_product(face1.m_normedEmbedding.begin(), face1.m_normedEmbedding.end(), face2.m_normedEmbedding.begin(), 0.0f);
        distance = 1.0f - dotProduct;
    }
    return distance;
}

bool FaceAnalyser::compareFace(const Face &face, const Face &referenceFace, const float &faceDistance) {
    float resultFaceDistance = calculateFaceDistance(face, referenceFace);
    return resultFaceDistance < faceDistance;
}

std::vector<Face> FaceAnalyser::findSimilarFaces(const std::vector<Face> &referenceFaces, const cv::Mat &targetVisionFrame, const float &faceDistance) {
    std::vector<Face> similarFaces;
    auto manyFaces = getManyFaces(targetVisionFrame);

    if (!manyFaces.empty()) {
        for (const auto &referenceFace : referenceFaces) {
            for (const auto &face : manyFaces) {
                if (compareFace(face, referenceFace, faceDistance)) {
                    similarFaces.push_back(face);
                }
            }
        }
    }
    return similarFaces;
}