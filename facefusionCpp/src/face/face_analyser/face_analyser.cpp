/**
 ******************************************************************************
 * @file           : face_analyser.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-3
 ******************************************************************************
 */

module;
#include <memory>
#include <tuple>
#include <iterator>
#include <numeric>
#include <stdexcept>

module face_analyser;
import face_helper;

namespace ffc {
FaceAnalyser::FaceAnalyser(const std::shared_ptr<Ort::Env> &env,
                           const InferenceSession::Options &_ISOptions) :
    env_(env), faceDetectorHub_(env, _ISOptions), faceLandMarkerHub_(env, _ISOptions),
    faceRecognizerHub_(env, _ISOptions), ISOptions_(_ISOptions) {
}

Face FaceAnalyser::getAverageFace(const std::vector<cv::Mat> &visionFrames, const Options &options) {
    if (visionFrames.empty()) {
        return {};
    }

    std::vector<Face> faces;
    for (const auto &visionFrame : visionFrames) {
        std::vector<Face> manyFaces = this->getManyFaces(visionFrame, options);
        faces.insert(faces.end(), manyFaces.begin(), manyFaces.end());
    }

    if (faces.empty()) {
        return {};
    }

    return getAverageFace(faces);
}

Face FaceAnalyser::getAverageFace(const std::vector<Face> &faces) {
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

        averageFace.m_embedding = FaceHelper::calcAverageEmbedding(embeddings);
        averageFace.m_normedEmbedding = FaceHelper::calcAverageEmbedding(normEmbeddings);
    }

    return averageFace;
}

Face FaceAnalyser::getOneFace(const cv::Mat &visionFrame, const Options &options, const unsigned int &position) {
    if (std::vector<Face> manyFaces = this->getManyFaces(visionFrame, options); !manyFaces.empty()) {
        if (position >= manyFaces.size()) {
            if (!manyFaces.empty()) {
                return manyFaces.back();
            }
            throw std::runtime_error("FaceAnalyser::getOneFace: position out of range");
        }
        return manyFaces.at(position);
    }
    return {};
}

std::vector<Face> FaceAnalyser::getManyFaces(const cv::Mat &visionFrame, const FaceAnalyser::Options &options) {
    if (faceStore_ == nullptr) {
        faceStore_ = std::make_shared<FaceStore>();
    }
    if (faceStore_->isContains(visionFrame)) {
        return faceStore_->getFaces(visionFrame);
    }

    std::vector<Face::BBox> resultBboxes;
    std::vector<Face::Landmark> resultLandmarks5;
    std::vector<Face::Score> resultScores;

    std::vector<FaceDetectorBase::Result> detectResults;
    double detectedAngle = 0;
    // 按逆时针方向每旋转90度探测一次，探测到脸或angle=270
    for (int angle = 0; angle < 360; angle += 90) {
        FaceDetectorHub::Options faceDetectorOptions = options.faceDetectorOptions;
        faceDetectorOptions.angle = angle;
        detectResults = faceDetectorHub_.detect(visionFrame, faceDetectorOptions);
        int emptyCount = 0;
        for (const auto &[bboxes, landmarks, scores] : detectResults) {
            if (bboxes.empty() || landmarks.empty() || scores.empty()) {
                emptyCount++;
            }
        }
        if (emptyCount < detectResults.size()) {
            detectedAngle = angle;
            break;
        }
    }
    for (auto &[bboxes, landmarks, scores] : detectResults) {
        resultBboxes.insert(resultBboxes.end(), std::make_move_iterator(bboxes.begin()), std::make_move_iterator(bboxes.end()));
        resultLandmarks5.insert(resultLandmarks5.end(), std::make_move_iterator(landmarks.begin()), std::make_move_iterator(landmarks.end()));
        resultScores.insert(resultScores.end(), std::make_move_iterator(scores.begin()), std::make_move_iterator(scores.end()));
    }
    if (resultBboxes.empty() || resultLandmarks5.empty() || resultScores.empty()) {
        return {};
    }

    std::vector<Face> resultFaces = createFaces(visionFrame, resultBboxes, resultLandmarks5, resultScores, detectedAngle, options);
    faceStore_->appendFaces(visionFrame, resultFaces);

    return resultFaces;
}

Face::Landmark FaceAnalyser::expandFaceLandmark68By5(const Face::Landmark &inputLandmark5) {
    return faceLandMarkerHub_.expandLandmark68By5(inputLandmark5);
}

std::vector<Face>
FaceAnalyser::createFaces(const cv::Mat &visionFrame, const std::vector<Face::BBox> &bBoxes,
                          const std::vector<Face::Landmark> &landmarks5,
                          const std::vector<Face::Score> &scores, const double &detectedAngle,
                          const Options &options) {
    std::vector<Face> resultFaces;
    if (options.faceDetectorOptions.minScore <= 0) {
        return resultFaces;
    }

    float iouThreshold = options.faceDetectorOptions.types.size() > 1 ? 0.1 : 0.4;
    std::vector<int> keepIndices = FaceHelper::applyNms(bBoxes, scores, iouThreshold);
    for (const auto &index : keepIndices) {
        Face tempFace;
        tempFace.m_bBox = bBoxes.at(index);
        tempFace.m_landmark5 = landmarks5.at(index);
        tempFace.m_landmark68By5 = expandFaceLandmark68By5(tempFace.m_landmark5);
        tempFace.m_detectorScore = scores.at(index);

        if (options.faceLandMarkerOptions.minScore > 0) {
            FaceLandmarkerHub::Options faceLandmarkerOptions = options.faceLandMarkerOptions;
            faceLandmarkerOptions.angle = detectedAngle;
            std::tie(tempFace.m_landmark68, tempFace.m_landmarkerScore) = faceLandMarkerHub_.detectLandmark68(visionFrame, tempFace.m_bBox, faceLandmarkerOptions);

            if (tempFace.m_landmarkerScore < options.faceLandMarkerOptions.minScore) {
                bool isFound = false;
                for (int angle = 90; angle < 360; angle += 90) {
                    faceLandmarkerOptions.angle = angle;
                    std::tie(tempFace.m_landmark68, tempFace.m_landmarkerScore) = faceLandMarkerHub_.detectLandmark68(visionFrame, tempFace.m_bBox, faceLandmarkerOptions);
                    if (tempFace.m_landmarkerScore > options.faceLandMarkerOptions.minScore) {
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

        std::array<Face::Embedding, 2> embeddingAndNormedEmbedding = this->calculateEmbedding(visionFrame, tempFace.m_landMark5By68, options.faceRecognizerType);
        tempFace.m_embedding = std::move(embeddingAndNormedEmbedding[0]);
        tempFace.m_normedEmbedding = std::move(embeddingAndNormedEmbedding[1]);

        std::tie(tempFace.m_gender, tempFace.m_age, tempFace.m_race) = this->classifyFace(visionFrame, tempFace.m_landMark5By68);

        resultFaces.emplace_back(tempFace);
    }

    if (resultFaces.empty()) {
        return {};
    }

    resultFaces = FaceSelector::select(resultFaces, options.faceSelectorOptions);
    return resultFaces;
}

std::array<Face::Embedding, 2>
FaceAnalyser::calculateEmbedding(const cv::Mat &visionFrame, const Face::Landmark &faceLandmark5By68,
                                 const FaceRecognizerHub::Type &type) {
    return faceRecognizerHub_.recognize(visionFrame, faceLandmark5By68, type);
}

std::tuple<Face::Gender, Face::Age, Face::Race> FaceAnalyser::classifyFace(const cv::Mat &visionFrame, const Face::Landmark &faceLandmarks5) {
    auto [race, gender, age] = faceClassifierHub_.classify(visionFrame, faceLandmarks5, FaceClassifierHub::Type::FairFace);
    return std::make_tuple(gender, age, race);
}

float FaceAnalyser::calculateFaceDistance(const Face &face1, const Face &face2) {
    float distance = 0.0f;
    if (!face1.m_normedEmbedding.empty() && !face2.m_normedEmbedding.empty()) {
        const float dotProduct = std::inner_product(face1.m_normedEmbedding.begin(), face1.m_normedEmbedding.end(), face2.m_normedEmbedding.begin(), 0.0f);
        distance = 1.0f - dotProduct;
    }
    return distance;
}

bool FaceAnalyser::compareFace(const Face &face, const Face &referenceFace, const float &faceDistance) {
    const float resultFaceDistance = calculateFaceDistance(face, referenceFace);
    return resultFaceDistance < faceDistance;
}

std::vector<Face> FaceAnalyser::findSimilarFaces(const std::vector<Face> &referenceFaces, const cv::Mat &targetVisionFrame, const float &faceDistance, const Options &options) {
    std::vector<Face> similarFaces;

    if (const auto manyFaces = getManyFaces(targetVisionFrame, options);
        !manyFaces.empty()) {
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
}