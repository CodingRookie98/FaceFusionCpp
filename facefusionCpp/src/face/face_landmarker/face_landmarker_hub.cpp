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

namespace ffc::face_landmarker {

using namespace ai::model_manager;
using namespace infra;

FaceLandmarkerHub::FaceLandmarkerHub(const std::shared_ptr<Ort::Env>& env, const ai::InferenceSession::Options& ISOptions) {
    m_env = env;
    m_is_options = ISOptions;
}

FaceLandmarkerBase* FaceLandmarkerHub::get_landmarker(const FaceLandmarkerHub::LandmarkerModel& type) {
    std::unique_lock lock(m_shared_mutex_landmarkers);
    if (m_landmarkers.contains(type)) {
        if (m_landmarkers[type] != nullptr) {
            return m_landmarkers[type];
        }
    }

    static std::shared_ptr<ModelManager> modelManager = ModelManager::get_instance();
    FaceLandmarkerBase* landmarker = nullptr;
    if (type == FaceLandmarkerHub::LandmarkerModel::_2DFAN) {
        landmarker = new T2dfan(m_env);
        landmarker->load_model(modelManager->get_model_path(Model::Face_landmarker_68), m_is_options);
    } else if (type == FaceLandmarkerHub::LandmarkerModel::_68By5) {
        landmarker = new T68By5(m_env);
        landmarker->load_model(modelManager->get_model_path(Model::Face_landmarker_68_5), m_is_options);
    } else if (type == FaceLandmarkerHub::LandmarkerModel::PEPPA_WUTZ) {
        landmarker = new Peppawutz(m_env);
        landmarker->load_model(modelManager->get_model_path(Model::Face_landmarker_peppawutz), m_is_options);
    }
    if (landmarker != nullptr) {
        m_landmarkers[type] = landmarker;
    }
    return m_landmarkers[type];
}

std::tuple<Face::Landmarks, float>
FaceLandmarkerHub::detect_landmark68(const cv::Mat& visionFrame, const BBox& bbox, const FaceLandmarkerHub::Options& options) {
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
        auto landmarker2dfan = dynamic_cast<T2dfan*>(get_landmarker(LandmarkerModel::_2DFAN));
        futures.emplace_back(ThreadPool::instance()->enqueue([&] {
            return landmarker2dfan->detect(rotatedVisionFrame, bbox);
        }));
    }
    if (options.types.contains(Type::PEPPA_WUTZ)) {
        auto landmarkerPeppawutz = dynamic_cast<Peppawutz*>(get_landmarker(LandmarkerModel::PEPPA_WUTZ));
        futures.emplace_back(ThreadPool::instance()->enqueue([&] {
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

Face::Landmarks FaceLandmarkerHub::expand_landmark68_from_5(const Face::Landmarks& landmark5) {
    auto landmarker68By5 = dynamic_cast<T68By5*>(get_landmarker(LandmarkerModel::_68By5));
    return landmarker68By5->detect(landmark5);
}

FaceLandmarkerHub::~FaceLandmarkerHub() {
    std::unique_lock<std::shared_mutex> lock(m_shared_mutex_landmarkers);
    for (auto& val : m_landmarkers | std::views::values) {
        delete val;
    }
    m_landmarkers.clear();
}
} // namespace ffc::face_landmarker