/**
 ******************************************************************************
 * @file           : face_landmarkers.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-16
 ******************************************************************************
 */

module;
#include <future>
#include <ranges>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

module face_landmarker_hub;
import :t2dfan;
import :t68by5;
import :peppawutz;
import face_helper;
import model_manager;
import thread_pool;

namespace ffc::faceLandmarker {
FaceLandmarkerHub::FaceLandmarkerHub(const std::shared_ptr<Ort::Env>& env, const ffc::InferenceSession::Options& ISOptions) {
    m_env = env;
    m_ISOptions = ISOptions;
}

FaceLandmarkerBase* FaceLandmarkerHub::getLandmarker(const FaceLandmarkerHub::LandmarkerModel& type) {
    std::unique_lock lock(m_sharedMutex);
    if (m_landmarkers.contains(type)) {
        if (m_landmarkers[type] != nullptr) {
            return m_landmarkers[type];
        }
    }

    static std::shared_ptr<ffc::ModelManager> modelManager = ffc::ModelManager::getInstance();
    FaceLandmarkerBase* landmarker = nullptr;
    if (type == FaceLandmarkerHub::LandmarkerModel::_2DFAN) {
        landmarker = new T2dfan(m_env);
        landmarker->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_landmarker_68), m_ISOptions);
    } else if (type == FaceLandmarkerHub::LandmarkerModel::_68By5) {
        landmarker = new T68By5(m_env);
        landmarker->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_landmarker_68_5), m_ISOptions);
    } else if (type == FaceLandmarkerHub::LandmarkerModel::PEPPA_WUTZ) {
        landmarker = new Peppawutz(m_env);
        landmarker->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_landmarker_peppawutz), m_ISOptions);
    }
    if (landmarker != nullptr) {
        m_landmarkers[type] = landmarker;
    }
    return m_landmarkers[type];
}

std::tuple<Face::Landmarks, float>
FaceLandmarkerHub::detectLandmark68(const cv::Mat& visionFrame, const Face::BBox& bbox, const FaceLandmarkerHub::Options& options) {
    std::vector<Face::Landmarks> landmarks;
    std::vector<float> scores;
    std::vector<std::future<std::tuple<Face::Landmarks, float>>> futures;
    cv::Mat rotatedInverseMat;
    cv::Mat rotatedVisionFrame;

    if (options.angle != 0) {
        auto [rotatedMat, rotatedSize] = face_helper::createRotatedMatAndSize(options.angle, visionFrame.size());
        cv::warpAffine(visionFrame, rotatedVisionFrame, rotatedMat, rotatedSize);
        cv::invertAffineTransform(rotatedMat, rotatedInverseMat);
    } else {
        rotatedVisionFrame = visionFrame;
    }

    if (options.types.contains(Type::_2DFAN)) {
        auto landmarker2dfan = dynamic_cast<T2dfan*>(getLandmarker(LandmarkerModel::_2DFAN));
        futures.emplace_back(ThreadPool::Instance()->Enqueue([&] {
            return landmarker2dfan->detect(rotatedVisionFrame, bbox);
        }));
    }
    if (options.types.contains(Type::PEPPA_WUTZ)) {
        auto landmarkerPeppawutz = dynamic_cast<Peppawutz*>(getLandmarker(LandmarkerModel::PEPPA_WUTZ));
        futures.emplace_back(ThreadPool::Instance()->Enqueue([&] {
            return landmarkerPeppawutz->detect(rotatedVisionFrame, bbox);
        }));
    }

    for (auto& future : futures) {
        auto [tempLandmark, tempScore] = future.get();
        if (options.angle != 0) {
            tempLandmark = face_helper::transformPoints(tempLandmark, rotatedInverseMat);
        }
        landmarks.push_back(tempLandmark);
        scores.push_back(tempScore);
    }

    if (options.types.size() >= 2 && landmarks.size() > 1) {
        if (scores[0] > scores[1] - 0.2) {
            return {landmarks[0], scores[0]};
        }
        return {landmarks[1], scores[1]};
    }
    return {landmarks[0], scores[0]};
}

Face::Landmarks FaceLandmarkerHub::expandLandmark68By5(const Face::Landmarks& landmark5) {
    auto landmarker68By5 = dynamic_cast<T68By5*>(getLandmarker(LandmarkerModel::_68By5));
    return landmarker68By5->detect(landmark5);
}

FaceLandmarkerHub::~FaceLandmarkerHub() {
    std::unique_lock<std::shared_mutex> lock(m_sharedMutex);
    for (auto& val : m_landmarkers | std::views::values) {
        delete val;
    }
    m_landmarkers.clear();
}
} // namespace ffc::faceLandmarker