/**
 ******************************************************************************
 * @file           : face_detectors.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

#include "face_detectors.h"
#include <future>
#include "face_detector_retina.h"
#include "face_detector_scrfd.h"
#include "face_detector_yolo.h"
#include "model_manager.h"

FaceDetectors::FaceDetectors(const std::shared_ptr<Ort::Env> &env) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectors");
    } else {
        m_env = env;
    }
}

FaceDetectors::~FaceDetectors() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    for (auto ptr : m_faceDetectors) {
        delete ptr.second;
    }
    m_faceDetectors.clear();
}

std::vector<FaceDetectorBase::Result>
FaceDetectors::detect(const cv::Mat &image, const cv::Size &faceDetectorSize,
                      const FaceDetectors::FaceDetectorType &type, const double &angle, const float &detectorScore) {
    std::vector<std::future<FaceDetectorBase::Result>> futures;
    if (type == FaceDetectorType::Many || type == FaceDetectorType::Retina) {
        createDetector(FaceDetectorType::Retina);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        FaceDetectorRetina *retina = dynamic_cast<FaceDetectorRetina *>(m_faceDetectors[FaceDetectorType::Retina]);
        if (retina != nullptr) {
            if (angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorRetina::detectRotatedFaces, retina, image, faceDetectorSize, angle, detectorScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorRetina::detectFaces, retina, image, faceDetectorSize, detectorScore));
            }
        }
    }
    if (type == FaceDetectorType::Many || type == FaceDetectorType::Scrfd) {
        createDetector(FaceDetectorType::Scrfd);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        FaceDetectorScrfd *scrfd = dynamic_cast<FaceDetectorScrfd *>(m_faceDetectors[FaceDetectorType::Scrfd]);
        if (scrfd != nullptr) {
            if (angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorScrfd::detectRotatedFaces, scrfd, image, faceDetectorSize, angle, detectorScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorScrfd::detectFaces, scrfd, image, faceDetectorSize, detectorScore));
            }
        }
    }
    if (type == FaceDetectorType::Many || type == FaceDetectorType::Yolo) {
        createDetector(FaceDetectorType::Yolo);
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        FaceDetectorYolo *yolo = dynamic_cast<FaceDetectorYolo *>(m_faceDetectors[FaceDetectorType::Yolo]);
        if (yolo != nullptr) {
            if (angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorYolo::detectRotatedFaces, yolo, image, faceDetectorSize, angle, detectorScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &FaceDetectorYolo::detectFaces, yolo, image, faceDetectorSize, detectorScore));
            }
        }
    }
    
    std::vector<FaceDetectorBase::Result> results;
    for (auto &future : futures) {
        results.emplace_back(future.get());
    }
    return results;
}

void FaceDetectors::createDetector(const FaceDetectors::FaceDetectorType &type) {
    if (m_faceDetectors.contains(type)) {
        if (m_faceDetectors[type] != nullptr) {
            return;
        }
    }

    static std::shared_ptr<Ffc::ModelManager> modelManager = Ffc::ModelManager::getInstance();
    FaceDetectorBase *detector = nullptr;
    if (type == FaceDetectorType::Retina) {
        detector = new FaceDetectorRetina(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_detector_retinaface));
    } else if (type == FaceDetectorType::Scrfd) {
        detector = new FaceDetectorScrfd(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_detector_scrfd));
    } else if (type == FaceDetectorType::Yolo) {
        detector = new FaceDetectorYolo(m_env, modelManager->getModelPath(Ffc::ModelManager::Model::Face_detector_yoloface));
    }
    if (detector != nullptr) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_faceDetectors[type] = detector;
    }
}
