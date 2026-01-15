#include <gtest/gtest.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <filesystem>
#include <vector>
#include <iostream>

import domain.face.swapper;
import domain.face.masker;
import domain.face.detector;
import domain.face.recognizer;
import domain.face; // Import base face module for types
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::face::swapper;
using namespace domain::face::masker;
using namespace foundation::infrastructure::test;

class FaceSwapperIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";

        repo = domain::ai::model_repository::ModelRepository::get_instance();
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        target_path = get_test_data_path("standard_face_test_images/tiffany.bmp");
    }

    // Helper: Detect face landmarks (5 points)
    domain::face::types::Landmarks get_face_landmarks(const cv::Mat& image) {
        if (image.empty()) return {};

        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::Yolo);

        std::string model_path = repo->ensure_model("face_detector_yoloface");
        if (model_path.empty()) return {};

        detector->load_model(model_path,
                             foundation::ai::inference_session::Options::with_best_providers());

        auto results = detector->detect(image);
        if (results.empty()) return {};

        // Return the first face's landmarks
        return results[0].landmarks;
    }

    // Helper: Get face embedding
    domain::face::types::Embedding get_face_embedding(
        const cv::Mat& image, const domain::face::types::Landmarks& landmarks) {
        if (image.empty() || landmarks.empty()) return {};

        auto recognizer = domain::face::recognizer::create_face_recognizer(
            domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);

        std::string model_path = repo->ensure_model("face_recognizer_arcface_w600k_r50");
        if (model_path.empty()) return {};

        recognizer->load_model(model_path,
                               foundation::ai::inference_session::Options::with_best_providers());

        auto result = recognizer->recognize(image, landmarks);
        return result.second; // Normalized embedding
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path target_path;
};

TEST_F(FaceSwapperIntegrationTest, SwapFaceAndVerifySimilarity) {
    if (!std::filesystem::exists(source_path) || !std::filesystem::exists(target_path)) {
        GTEST_SKIP() << "Test images not found";
    }

    cv::Mat source_img = cv::imread(source_path.string());
    cv::Mat target_img = cv::imread(target_path.string());

    ASSERT_FALSE(source_img.empty());
    ASSERT_FALSE(target_img.empty());

    // 1. Extract Source Embedding
    auto source_kps = get_face_landmarks(source_img);
    ASSERT_FALSE(source_kps.empty()) << "No face detected in source image";
    auto source_embedding = get_face_embedding(source_img, source_kps);
    ASSERT_FALSE(source_embedding.empty()) << "Failed to extract source embedding";

    // 2. Prepare Target
    auto target_kps = get_face_landmarks(target_img);
    ASSERT_FALSE(target_kps.empty()) << "No face detected in target image";

    // 3. Run Swapper
    auto swapper = FaceSwapperFactory::create_inswapper();
    // Correct key from models_info.json is "inswapper_128"
    std::string swapper_model_path = repo->ensure_model("inswapper_128");
    ASSERT_FALSE(swapper_model_path.empty()) << "Swapper model not found";

    swapper->load_model(swapper_model_path,
                        foundation::ai::inference_session::Options::with_best_providers());

    SwapInput input;
    input.target_frame = target_img;
    input.source_embedding = source_embedding;
    input.target_faces_landmarks = {target_kps};
    input.mask_options.mask_types = {MaskType::Box}; // Use basic mask for now

    cv::Mat result_img = swapper->swap_face(input);
    ASSERT_FALSE(result_img.empty());

    // 4. Verify Result
    // Extract embedding from result face
    auto result_kps = get_face_landmarks(result_img);
    ASSERT_FALSE(result_kps.empty()) << "No face detected in result image";
    auto result_embedding = get_face_embedding(result_img, result_kps);

    // Calculate Cosine Similarity with Source Embedding
    // Sim = dot(A, B) / (norm(A)*norm(B))
    // Since embeddings are normalized, just dot product
    double similarity = 0.0;
    for (size_t i = 0; i < source_embedding.size(); ++i) {
        similarity += source_embedding[i] * result_embedding[i];
    }

    std::cout << "Swap Similarity: " << similarity << std::endl;

    // Expect reasonable similarity (usually > 0.3 or 0.4 for swap result vs source)
    EXPECT_GT(similarity, 0.3) << "Swapped face should resemble source face";

    // Save result for visual inspection
    std::filesystem::create_directories("tests_output");
    cv::imwrite("tests_output/swap_test_result.jpg", result_img);
}
