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
#include <onnxruntime_cxx_api.h>
#include <opencv2/opencv.hpp>

module face_detector_hub;
import :yolo;
import :retina;
import :scrfd;
import model_manager;
import thread_pool;

namespace ffc::faceDetector {

FaceDetectorHub::FaceDetectorHub(const std::shared_ptr<Ort::Env>& env, const InferenceSession::Options& ISOptions) {
    if (env == nullptr) {
        env_ = std::make_shared<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "FaceDetectorHub");
    } else {
        env_ = env;
    }
    inference_session_options_ = ISOptions;
}

FaceDetectorHub::~FaceDetectorHub() = default;

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
                                          [](const cv::Size& size1, const cv::Size& size2) -> bool {
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
FaceDetectorHub::Detect(const cv::Mat& image, const Options& options) {
    std::vector<std::future<FaceDetectorBase::Result>> futures;

    if (options.types.contains(Type::Retina)) {
        const auto retina = std::dynamic_pointer_cast<Retina>(GetDetector(Type::Retina));
        if (retina != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Retina::DetectRotatedFaces, retina, image, options.face_detector_size, options.angle, options.min_score)));
            } else {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Retina::DetectFaces, retina, image, options.face_detector_size, options.min_score)));
            }
        }
    }

    if (options.types.contains(Type::Scrfd)) {
        const auto scrfd = std::dynamic_pointer_cast<Scrfd>(GetDetector(Type::Scrfd));
        if (scrfd != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Scrfd::DetectRotatedFaces, scrfd, image, options.face_detector_size, options.angle, options.min_score)));
            } else {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Scrfd::DetectFaces, scrfd, image, options.face_detector_size, options.min_score)));
            }
        }
    }

    if (options.types.contains(Type::Yolo)) {
        const auto yolo = std::dynamic_pointer_cast<Yolo>(GetDetector(Type::Yolo));
        if (yolo != nullptr) {
            if (options.angle > 0) {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Yolo::DetectRotatedFaces, yolo, image, options.face_detector_size, options.angle, options.min_score)));
            } else {
                futures.emplace_back(ThreadPool::Instance()->Enqueue(std::bind(&Yolo::DetectFaces, yolo, image, options.face_detector_size, options.min_score)));
            }
        }
    }

    std::vector<FaceDetectorBase::Result> results;
    for (auto& future : futures) {
        results.emplace_back(future.get());
    }

    return results;
}

std::shared_ptr<FaceDetectorBase> FaceDetectorHub::GetDetector(const Type& type) {
    std::unique_lock lock(shared_mutex_);
    if (face_detectors_.contains(type)) {
        if (face_detectors_[type] != nullptr) {
            return face_detectors_[type];
        }
    }

    static std::shared_ptr<ffc::ModelManager> modelManager = ffc::ModelManager::getInstance();
    std::shared_ptr<FaceDetectorBase> detector{nullptr};
    if (type == Type::Retina) {
        detector = std::make_shared<Retina>(env_);
        detector->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_retinaface), inference_session_options_);
    } else if (type == Type::Scrfd) {
        detector = std::make_shared<Scrfd>(env_);
        detector->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_scrfd), inference_session_options_);
    } else if (type == Type::Yolo) {
        detector = std::make_shared<Yolo>(env_);
        detector->LoadModel(modelManager->getModelPath(ffc::ModelManager::Model::Face_detector_yoloface), inference_session_options_);
    }

    face_detectors_[type] = detector;
    return face_detectors_[type];
}
} // namespace ffc::faceDetector