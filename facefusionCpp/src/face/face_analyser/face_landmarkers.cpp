/**
 ******************************************************************************
 * @file           : face_landmarkers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

#include "face_landmarkers.h"
#include "model_manager.h"
#include "face_landmarker_2dfan.h"
#include "face_landmarker_peppawutz.h"
#include "face_landmarker_68By5.h"
#include "face_helper.h"
#include <future>

FaceLandmarkers::FaceLandmarkers(const std::shared_ptr<Ort::Env> &env) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectors");
    } else {
        m_env = env;
    }
}
void FaceLandmarkers::createLandmarker(const FaceLandmarkers::LandmarkerModel &type) {
    if (m_landmarkers.contains(type)) {
        if (m_landmarkers[type] != nullptr) {
            return;
        }
    }

    static std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    FaceLandmarkerBase *landmarker = nullptr;
    if (type == FaceLandmarkers::LandmarkerModel::_2DFAN) {
        landmarker = new FaceLandmarker2dfan(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_landmarker_68));
    } else if (type == FaceLandmarkers::LandmarkerModel::_68By5) {
        landmarker = new FaceLandmarker68By5(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_landmarker_68_5));
    } else if (type == FaceLandmarkers::LandmarkerModel::PEPPA_WUTZ) {
        landmarker = new FaceLandmarkerPeppawutz(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_landmarker_peppawutz));
    }
    if (landmarker != nullptr) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_landmarkers[type] = landmarker;
    }
}

std::tuple<Face::Landmark, float>
FaceLandmarkers::detectLandmark68(const cv::Mat &image, const Face::BBox &Bbox, const Landmarker68Model &model) {
    std::vector<Face::Landmark> landmarks;
    std::vector<float> scores;
    std::vector<std::future<std::tuple<Face::Landmark, float>>> futures;
    if (model == Landmarker68Model::Many || model == Landmarker68Model::_2DFAN) {
        createLandmarker(LandmarkerModel::_2DFAN);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        FaceLandmarker2dfan *landmarker2dfan = dynamic_cast<FaceLandmarker2dfan *>(m_landmarkers[LandmarkerModel::_2DFAN]);
        futures.emplace_back(std::async(std::launch::async, &FaceLandmarker2dfan::detect, landmarker2dfan, image, Bbox));
    }
    if (model == Landmarker68Model::Many || model == Landmarker68Model::PEPPA_WUTZ) {
        createLandmarker(LandmarkerModel::PEPPA_WUTZ);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        FaceLandmarkerPeppawutz *landmarkerPeppawutz = dynamic_cast<FaceLandmarkerPeppawutz *>(m_landmarkers[LandmarkerModel::PEPPA_WUTZ]);
        futures.emplace_back(std::async(std::launch::async, &FaceLandmarkerPeppawutz::detect, landmarkerPeppawutz, image, Bbox));
    }
    for (auto &future : futures) {
        auto [tempLandmark, tempScore] = future.get();
        landmarks.push_back(tempLandmark);
        scores.push_back(tempScore);
    }
    if (model == Landmarker68Model::Many && landmarks.size() > 1) {
        if (scores[0] > scores[1] - 0.2) {
            return {landmarks[0], scores[0]};
        } else {
            return {landmarks[1], scores[1]};
        }
    }
    return {landmarks[0], scores[0]};
}

std::tuple<Face::Landmark, float> FaceLandmarkers::detectLandmark68(const cv::Mat &image, const Face::BBox &Bbox, const double &angle, const FaceLandmarkers::Landmarker68Model &model) {
    cv::Mat rotatedMat, rotatedVisionFrame, rotatedInverseMat;
    cv::Size rotatedSize;
    std::tie(rotatedMat, rotatedSize) = FaceHelper::createRotatedMatAndSize(angle, image.size());
    cv::warpAffine(image, rotatedVisionFrame, rotatedMat, rotatedSize);
    cv::invertAffineTransform(rotatedMat, rotatedInverseMat);

    auto [landmark, score] = detectLandmark68(rotatedVisionFrame, Bbox, model);
    landmark = FaceHelper::transformPoints(landmark, rotatedInverseMat);
    return {landmark, score};
}

Face::Landmark FaceLandmarkers::expandLandmark68By5(const Face::Landmark &landmark5) {
    createLandmarker(LandmarkerModel::_68By5);
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    FaceLandmarker68By5 *landmarker68By5 = dynamic_cast<FaceLandmarker68By5 *>(m_landmarkers[LandmarkerModel::_68By5]);
    return landmarker68By5->detect(landmark5);
}

FaceLandmarkers::~FaceLandmarkers() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto &landmarker : m_landmarkers) {
        delete landmarker.second;
    }
    m_landmarkers.clear();
}
