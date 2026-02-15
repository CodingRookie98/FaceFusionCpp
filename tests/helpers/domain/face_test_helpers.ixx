module;
#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>
#include <string>
#include <filesystem>

/**
 * @file face_test_support.ixx
 * @brief Test support utilities for face module
 * @details Provides helper functions for creating test faces and detecting faces in images.
 *          This module is intended for test code only - production code should not depend on it.
 * @author CodingRookie
 * @date 2026-01-18
 */
export module tests.helpers.domain.face_test_helpers;

import domain.face.analyser;
export import domain.face;
import domain.face.detector;
import domain.face.recognizer;
import domain.ai.model_repository;
import foundation.ai.inference_session;

export namespace tests::helpers::domain {

using namespace ::domain::face;

/**
 * @brief Create an empty face object for testing
 * @return Empty Face instance
 */
Face create_empty_face() {
    return Face();
}

/**
 * @brief Create a test face with basic properties
 * @return Face instance with 5-point landmarks and default scores
 */
Face create_test_face() {
    Face face;
    face.set_box({10.0f, 10.0f, 100.0f, 100.0f});

    types::Landmarks kps;
    for (int i = 0; i < 5; ++i) {
        kps.emplace_back(static_cast<float>(10.0f + i * 10), static_cast<float>(10.0f + i * 10));
    }
    face.set_kps(std::move(kps));

    face.set_detector_score(0.95f);
    face.set_landmarker_score(0.98f);

    return face;
}

/**
 * @brief Create a test face with 68-point landmarks
 * @return Face instance with 68-point landmarks
 */
Face create_face_with_68_kps() {
    Face face;
    face.set_box({0.0f, 0.0f, 200.0f, 200.0f});

    types::Landmarks kps;
    kps.reserve(68);
    for (int i = 0; i < 68; ++i) { kps.emplace_back(static_cast<float>(i), static_cast<float>(i)); }
    face.set_kps(std::move(kps));

    return face;
}

/**
 * @brief Setup model repository with assets path
 * @param assets_path Path to assets directory containing models_info.json
 * @return Configured ModelRepository instance
 */
inline std::shared_ptr<::domain::ai::model_repository::ModelRepository> setup_model_repository(
    const std::filesystem::path& assets_path) {
    auto repo = ::domain::ai::model_repository::ModelRepository::get_instance();
    auto models_info_path = assets_path / "models_info.json";
    if (std::filesystem::exists(models_info_path)) {
        repo->set_model_info_file_path(models_info_path.string());
    }
    return repo;
}

/**
 * @brief Detect face landmarks from an image using YoloFace detector
 * @param image Input image (BGR format)
 * @param repo Model repository instance
 * @return Vector of 2D points representing face landmarks (5 points), empty if no face detected
 */
inline types::Landmarks detect_face_landmarks(
    const cv::Mat& image, std::shared_ptr<::domain::ai::model_repository::ModelRepository> repo) {
    if (image.empty()) return {};

    auto detector_ptr = ::domain::face::detector::FaceDetectorFactory::create(
        ::domain::face::detector::DetectorType::Yolo);

    std::string model_path = repo->ensure_model("yoloface");
    if (model_path.empty()) return {};

    detector_ptr->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

    auto results = detector_ptr->detect(image);
    if (results.empty()) return {};

    return results[0].landmarks;
}

/**
 * @brief Detect face bounding box from an image
 * @param image Input image (BGR format)
 * @param repo Model repository instance
 * @return Bounding box of detected face, empty rect if no face detected
 */
inline cv::Rect2f detect_face_bbox(
    const cv::Mat& image, std::shared_ptr<::domain::ai::model_repository::ModelRepository> repo) {
    if (image.empty()) return {};

    auto detector_ptr = ::domain::face::detector::FaceDetectorFactory::create(
        ::domain::face::detector::DetectorType::SCRFD);

    std::string model_path = repo->ensure_model("scrfd");
    if (model_path.empty()) return {};

    detector_ptr->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

    auto results = detector_ptr->detect(image);
    if (results.empty()) return {};

    return results[0].box;
}

/**
 * @brief Get face embedding from an image
 * @param image Input image (BGR format)
 * @param landmarks Face landmarks (5 points)
 * @param repo Model repository instance
 * @return Normalized face embedding vector, empty if failed
 */
inline types::Embedding get_face_embedding(
    const cv::Mat& image, const types::Landmarks& landmarks,
    std::shared_ptr<::domain::ai::model_repository::ModelRepository> repo) {
    if (image.empty() || landmarks.empty()) return {};

    auto recognizer_ptr = ::domain::face::recognizer::create_face_recognizer(
        ::domain::face::recognizer::FaceRecognizerType::ArcFaceW600kR50);

    std::string model_path = repo->ensure_model("arcface_w600k_r50");
    if (model_path.empty()) return {};

    recognizer_ptr->load_model(model_path,
                               foundation::ai::inference_session::Options::with_best_providers());

    auto result = recognizer_ptr->recognize(image, landmarks);
    return result.second; // Normalized embedding
}

/**
 * @brief Create a configured FaceAnalyser for testing (Shared by Image/Video Integration
 * Tests)
 * @param repo Model repository instance
 * @return Shared pointer to FaceAnalyser
 */
inline std::shared_ptr<::domain::face::analyser::FaceAnalyser> create_face_analyser(
    std::shared_ptr<::domain::ai::model_repository::ModelRepository> repo) {
    using namespace ::domain::face::analyser;
    Options opts;
    opts.inference_session_options =
        foundation::ai::inference_session::Options::with_best_providers();

    // Configure default paths from repo
    opts.model_paths.face_detector_yolo = repo->ensure_model("yoloface");
    opts.model_paths.face_recognizer_arcface = repo->ensure_model("arcface_w600k_r50");
    // We only need detection and embedding for similarity check

    opts.face_detector_options.type = ::domain::face::detector::DetectorType::Yolo;
    opts.face_recognizer_type = ::domain::face::recognizer::FaceRecognizerType::ArcFaceW600kR50;

    return std::make_shared<FaceAnalyser>(opts);
}

} // namespace tests::helpers::domain
