/**
 ******************************************************************************
 * @file           : face_detectors.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-10-14
 ******************************************************************************
 */

module;
#include <memory>
#include <future>
#include <ranges>
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

module face_detector_hub;
import :yolo;
import :retina;
import :scrfd;
import model_manager;

namespace ffc::faceDetector {

FaceDetectorHub::FaceDetectorHub(const std::shared_ptr<Ort::Env> &env, const InferenceSession::Options &ISOptions) {
    if (env == nullptr) {
        m_env = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectorHub");
    } else {
        m_env = env;
    }
    m_ISOptions = ISOptions;
}

FaceDetectorHub::~FaceDetectorHub() {
    std::unique_lock lock(m_sharedMutex);
    for (auto val : m_faceDetectors | std::views::values) {
        delete val;
        val = nullptr;
    }
    m_faceDetectors.clear();
}

std::vector<cv::Size> FaceDetectorHub::GetSupportSizes(const Type& type) {
    switch (type) {
    case Type::Retina:
        return Retina::GetSupportSizes();
    case Type::Scrfd:
        return Scrfd::GetSupportSizes();
    case Type::Yolo:
        return Yolo::GetSupportSizes();
    }
    return {{640, 640}};
}

std::vector<cv::Size> FaceDetectorHub::GetSupportCommonSizes(const std::unordered_set<Type>& types) {
    std::vector<cv::Size> commonSizes;
    for (auto type : types) {
        auto sizes = GetSupportSizes(type);
        if (commonSizes.empty()) {
            commonSizes = sizes;
        } else {
            std::vector<cv::Size> intersection;
            std::ranges::set_intersection(commonSizes, sizes, std::inserter(intersection, intersection.begin()),
                [](const cv::Size& size1, const cv::Size& size2)-> bool {
                    if (size1.area() < size2.area()) {
                        return true;
                    }
                    return false;
                });
            commonSizes = intersection;
        }
    }
    return commonSizes;
}

std::vector<FaceDetectorBase::Result>
FaceDetectorHub::detect(const cv::Mat &image, const Options &options) {
    std::vector<std::future<FaceDetectorBase::Result>> futures;

    if (options.types.contains(Type::Retina)) {
        auto retina = dynamic_cast<Retina *>(getDetector(Type::Retina));
        if (retina != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &Retina::detectRotatedFaces, retina, image, options.faceDetectorSize, options.angle, options.minScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &Retina::detectFaces, retina, image, options.faceDetectorSize, options.minScore));
            }
        }
    }

    if (options.types.contains(Type::Scrfd)) {
        auto scrfd = dynamic_cast<Scrfd *>(getDetector(Type::Scrfd));
        if (scrfd != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &Scrfd::detectRotatedFaces, scrfd, image, options.faceDetectorSize, options.angle, options.minScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &Scrfd::detectFaces, scrfd, image, options.faceDetectorSize, options.minScore));
            }
        }
    }

    if (options.types.contains(Type::Yolo)) {
        auto yolo = dynamic_cast<Yolo *>(getDetector(Type::Yolo));
        if (yolo != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(std::async(std::launch::async, &Yolo::detectRotatedFaces, yolo, image, options.faceDetectorSize, options.angle, options.minScore));
            } else {
                futures.emplace_back(std::async(std::launch::async, &Yolo::detectFaces, yolo, image, options.faceDetectorSize, options.minScore));
            }
        }
    }

    std::vector<FaceDetectorBase::Result> results;
    for (auto &future : futures) {
        results.emplace_back(future.get());
    }
    return results;
}

FaceDetectorBase *FaceDetectorHub::getDetector(const Type &type) {
    std::unique_lock lock(m_sharedMutex);
    if (m_faceDetectors.contains(type)) {
        if (m_faceDetectors[type] != nullptr) {
            return m_faceDetectors[type];
        }
    }

    static std::shared_ptr<ffc::ModelManager> modelManager = ffc::ModelManager::getInstance();
    FaceDetectorBase *detector = nullptr;
    if (type == Type::Retina) {
        detector = dynamic_cast<FaceDetectorBase *>(new Retina(m_env));
        detector->loadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_retinaface), m_ISOptions);
    } else if (type == Type::Scrfd) {
        detector = new Scrfd(m_env);
        detector->loadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_scrfd), m_ISOptions);
    } else if (type == Type::Yolo) {
        detector = new Yolo(m_env);
        detector->loadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_yoloface), m_ISOptions);
    }

    m_faceDetectors[type] = detector;
    return m_faceDetectors[type];
}
} // namespace ffc::faceDetector