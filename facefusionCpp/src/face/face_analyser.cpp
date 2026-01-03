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
using namespace ffc::ai;
FaceAnalyser::FaceAnalyser(const std::shared_ptr<Ort::Env>& env,
                           const InferenceSession::Options& _ISOptions) :
    env_(env), faceDetectorHub_(env, _ISOptions), faceLandMarkerHub_(env, _ISOptions),
    faceRecognizerHub_(env, _ISOptions), ISOptions_(_ISOptions) {
    if (faceStore_ == nullptr) {
        faceStore_ = std::make_shared<FaceStore>();
    }
}

Face FaceAnalyser::GetAverageFace(const std::vector<cv::Mat>& visionFrames, const Options& options) {
    if (visionFrames.empty()) {
        return {};
    }

    std::vector<Face> faces;
    for (const auto& visionFrame : visionFrames) {
        std::vector<Face> manyFaces = this->GetManyFaces(visionFrame, options);
        faces.insert(faces.end(), manyFaces.begin(), manyFaces.end());
    }

    if (faces.empty()) {
        return {};
    }

    return GetAverageFace(faces);
}

Face FaceAnalyser::GetAverageFace(const std::vector<Face>& faces) {
    if (faces.empty()) {
        return {};
    }

    Face averageFace = *std::ranges::find_if(faces, [](const Face& face) {
        return !face.is_empty();
    });
    if (faces.size() > 1) {
        std::vector<Face::Embedding> embeddings, normEmbeddings;
        for (auto& face : faces) {
            embeddings.push_back(std::move(face.m_embedding));
            normEmbeddings.push_back(std::move(face.m_normed_embedding));
        }

        averageFace.m_embedding = face_helper::calcAverageEmbedding(embeddings);
        averageFace.m_normed_embedding = face_helper::calcAverageEmbedding(normEmbeddings);
    }

    return averageFace;
}

Face FaceAnalyser::GetOneFace(const cv::Mat& visionFrame, const Options& options, const unsigned int& position) {
    if (std::vector<Face> manyFaces = this->GetManyFaces(visionFrame, options); !manyFaces.empty()) {
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

std::vector<Face> FaceAnalyser::GetManyFaces(const cv::Mat& visionFrame, const FaceAnalyser::Options& options) {
    const std::string k_faces_name = faceStore_->CreateFrameHash(visionFrame);
    if (faceStore_->IsContains(k_faces_name)) {
        return faceStore_->GetFaces(k_faces_name);
    }

    std::vector<BBox> resultBboxes;
    std::vector<Face::Landmarks> resultLandmarks5;
    std::vector<Face::Score> resultScores;

    std::vector<FaceDetectorBase::Result> detectResults;
    double detectedAngle = 0;
    // 按逆时针方向每旋转90度探测一次，探测到脸或angle=270
    for (int angle = 0; angle < 360; angle += 90) {
        FaceDetectorHub::Options faceDetectorOptions = options.faceDetectorOptions;
        faceDetectorOptions.angle = angle;
        detectResults = faceDetectorHub_.Detect(visionFrame, faceDetectorOptions);
        int emptyCount = 0;
        for (const auto& [bboxes, landmarks, scores] : detectResults) {
            if (bboxes.empty() || landmarks.empty() || scores.empty()) {
                emptyCount++;
            }
        }
        if (emptyCount < detectResults.size()) {
            detectedAngle = angle;
            break;
        }
    }
    for (auto& [bboxes, landmarks, scores] : detectResults) {
        resultBboxes.insert(resultBboxes.end(), std::make_move_iterator(bboxes.begin()), std::make_move_iterator(bboxes.end()));
        resultLandmarks5.insert(resultLandmarks5.end(), std::make_move_iterator(landmarks.begin()), std::make_move_iterator(landmarks.end()));
        resultScores.insert(resultScores.end(), std::make_move_iterator(scores.begin()), std::make_move_iterator(scores.end()));
    }
    if (resultBboxes.empty() || resultLandmarks5.empty() || resultScores.empty()) {
        return {};
    }

    std::vector<Face> resultFaces = CreateFaces(visionFrame, resultBboxes, resultLandmarks5, resultScores, detectedAngle, options);

    faceStore_->InsertFaces(k_faces_name, resultFaces);

    return resultFaces;
}

Face::Landmarks FaceAnalyser::ExpandFaceLandmarks68From5(const Face::Landmarks& inputLandmark5) {
    return faceLandMarkerHub_.expand_landmark68_from_5(inputLandmark5);
}

std::vector<Face>
FaceAnalyser::CreateFaces(const cv::Mat& visionFrame, const std::vector<BBox>& bBoxes,
                          const std::vector<Face::Landmarks>& landmarks5,
                          const std::vector<Face::Score>& scores, const double& detectedAngle,
                          const Options& options) {
    std::vector<Face> resultFaces;
    if (options.faceDetectorOptions.min_score <= 0) {
        return resultFaces;
    }

    float iouThreshold           = options.faceDetectorOptions.types.size() > 1 ? 0.1 : 0.4;
    std::vector<int> keepIndices = face_helper::applyNms(bBoxes, scores, iouThreshold);
    for (const auto& index : keepIndices) {
        Face tempFace;
        tempFace.m_box               = bBoxes.at(index);
        tempFace.m_landmark5 = landmarks5.at(index);
        tempFace.m_landmark68_from_5 = ExpandFaceLandmarks68From5(tempFace.m_landmark5);
        tempFace.m_detector_score = scores.at(index);

        if (options.faceLandMarkerOptions.minScore > 0) {
            FaceLandmarkerHub::Options faceLandmarkerOptions             = options.faceLandMarkerOptions;
            faceLandmarkerOptions.angle = detectedAngle;
            std::tie(tempFace.m_landmark68, tempFace.m_landmarker_score) = faceLandMarkerHub_.detect_landmark68(visionFrame, tempFace.m_box, faceLandmarkerOptions);

            if (tempFace.m_landmarker_score < options.faceLandMarkerOptions.minScore) {
                bool isFound = false;
                for (int angle = 90; angle < 360; angle += 90) {
                    faceLandmarkerOptions.angle                                  = angle;
                    std::tie(tempFace.m_landmark68, tempFace.m_landmarker_score) = faceLandMarkerHub_.detect_landmark68(visionFrame, tempFace.m_box, faceLandmarkerOptions);
                    if (tempFace.m_landmarker_score > options.faceLandMarkerOptions.minScore) {
                        tempFace.m_landmark5_from_68 = face_helper::convertFaceLandmark68To5(tempFace.m_landmark68);
                        isFound = true;
                        break;
                    }
                }
                if (!isFound) {
                    tempFace.m_landmark68 = tempFace.m_landmark68_from_5;
                    tempFace.m_landmark5_from_68 = tempFace.m_landmark5;
                    tempFace.m_landmarker_score = 0;
                }
            } else {
                tempFace.m_landmark5_from_68 = face_helper::convertFaceLandmark68To5(tempFace.m_landmark68);
            }
        }

        std::array<Face::Embedding, 2> embeddingAndNormedEmbedding = this->CalculateEmbedding(visionFrame, tempFace.m_landmark5_from_68, options.faceRecognizerType);
        tempFace.m_embedding = std::move(embeddingAndNormedEmbedding[0]);
        tempFace.m_normed_embedding = std::move(embeddingAndNormedEmbedding[1]);

        std::tie(tempFace.m_gender, tempFace.m_age_range, tempFace.m_race) = this->ClassifyFace(visionFrame, tempFace.m_landmark5_from_68);

        resultFaces.emplace_back(tempFace);
    }

    if (resultFaces.empty()) {
        return {};
    }

    resultFaces = FaceSelector::select(resultFaces, options.faceSelectorOptions);
    return resultFaces;
}

std::array<Face::Embedding, 2>
FaceAnalyser::CalculateEmbedding(const cv::Mat& visionFrame, const Face::Landmarks& faceLandmark5By68,
                                 const FaceRecognizerHub::Type& type) {
    return faceRecognizerHub_.recognize(visionFrame, faceLandmark5By68, type);
}

std::tuple<Gender, AgeRange, Race> FaceAnalyser::ClassifyFace(const cv::Mat& visionFrame, const Face::Landmarks& faceLandmarks5) {
    auto [race, gender, ageRange] = faceClassifierHub_.classify(visionFrame, faceLandmarks5, FaceClassifierHub::Type::FairFace);
    return std::make_tuple(gender, ageRange, race);
}

float FaceAnalyser::CalculateFaceDistance(const Face& face1, const Face& face2) {
    float distance = 0.0f;
    if (!face1.m_normed_embedding.empty() && !face2.m_normed_embedding.empty()) {
        const float dotProduct = std::inner_product(face1.m_normed_embedding.begin(), face1.m_normed_embedding.end(), face2.m_normed_embedding.begin(), 0.0f);
        distance = 1.0f - dotProduct;
    }
    return distance;
}

bool FaceAnalyser::CompareFace(const Face& face, const Face& referenceFace, const float& faceDistance) {
    const float resultFaceDistance = CalculateFaceDistance(face, referenceFace);
    return resultFaceDistance < faceDistance;
}

std::vector<Face> FaceAnalyser::FindSimilarFaces(const std::vector<Face>& referenceFaces, const cv::Mat& targetVisionFrame, const float& faceDistance, const Options& options) {
    std::vector<Face> similarFaces;

    if (const auto manyFaces = GetManyFaces(targetVisionFrame, options);
        !manyFaces.empty()) {
        for (const auto& referenceFace : referenceFaces) {
            for (const auto& face : manyFaces) {
                if (CompareFace(face, referenceFace, faceDistance)) {
                    similarFaces.push_back(face);
                }
            }
        }
    }
    return similarFaces;
}
} // namespace ffc