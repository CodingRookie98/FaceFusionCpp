/**
 * @file face_detector_benchmark.cpp
 * @brief Benchmark tests for FaceDetector to measure detection performance
 * @author hermes
 * @date 2026-05-30
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <chrono>
#include <vector>
#include <opencv2/opencv.hpp>

import domain.face.detector;
import domain.ai.model_repository;
import foundation.infrastructure.test_support;

using namespace domain::face::detector;
using namespace foundation::infrastructure::test;

class FaceDetectorBenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        test_images = {get_test_data_path("standard_face_test_images/lenna.bmp"),
                       get_test_data_path("standard_face_test_images/tiffany.bmp"),
                       get_test_data_path("standard_face_test_images/girl.bmp")};
    }

    struct BenchmarkResult {
        std::string model_name;
        double avg_time_ms;
        int faces_detected;
    };

    BenchmarkResult benchmark_detector(DetectorType type, const std::string& model_key,
                                       const std::string& model_name, int iterations = 5) {
        BenchmarkResult result{model_name, 0.0, 0};

        try {
            auto detector = FaceDetectorFactory::create(type);
            if (!detector) {
                std::cout << "[SKIP] Failed to create detector: " << model_name << std::endl;
                return result;
            }

            std::string model_path = repo->ensure_model(model_key);
            if (model_path.empty()) {
                std::cout << "[SKIP] Model not available: " << model_key << std::endl;
                return result;
            }

            detector->load_model(model_path, Options::with_best_providers());

            // Warm up
            for (const auto& img_path : test_images) {
                if (!std::filesystem::exists(img_path)) continue;
                cv::Mat frame = cv::imread(img_path.string());
                if (frame.empty()) continue;
                detector->detect(frame);
            }

            // Benchmark
            std::vector<double> times;
            int total_faces = 0;

            for (int i = 0; i < iterations; ++i) {
                for (const auto& img_path : test_images) {
                    if (!std::filesystem::exists(img_path)) continue;
                    cv::Mat frame = cv::imread(img_path.string());
                    if (frame.empty()) continue;

                    auto start = std::chrono::high_resolution_clock::now();
                    auto faces = detector->detect(frame);
                    auto end = std::chrono::high_resolution_clock::now();

                    double time_ms =
                        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
                        / 1000.0;
                    times.push_back(time_ms);
                    total_faces += faces.size();
                }
            }

            if (!times.empty()) {
                result.avg_time_ms =
                    std::accumulate(times.begin(), times.end(), 0.0) / times.size();
                result.faces_detected = total_faces / iterations;
            }
        } catch (const std::exception& e) {
            std::cout << "[ERROR] " << model_name << ": " << e.what() << std::endl;
        }

        return result;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::vector<std::filesystem::path> test_images;
};

TEST_F(FaceDetectorBenchmarkTest, BenchmarkAllDetectors) {
    std::cout << "\n=======================================================" << std::endl;
    std::cout << "[BENCHMARK] Face Detector Performance Comparison" << std::endl;
    std::cout << "=======================================================" << std::endl;

    struct DetectorConfig {
        DetectorType type;
        std::string model_key;
        std::string model_name;
    };

    std::vector<DetectorConfig> detectors = {{DetectorType::Yolo, "yoloface", "YOLOFace"},
                                             {DetectorType::RetinaFace, "retinaface", "RetinaFace"},
                                             {DetectorType::SCRFD, "scrfd", "SCRFD"}};

    std::vector<BenchmarkResult> results;

    for (const auto& config : detectors) {
        std::cout << "\n[Benchmarking] " << config.model_name << "..." << std::endl;
        auto result = benchmark_detector(config.type, config.model_key, config.model_name);
        results.push_back(result);
    }

    // Print results table
    std::cout << "\n+---------------------+--------------+----------------+" << std::endl;
    std::cout << "| Model               | Avg Time (ms)| Faces Detected |" << std::endl;
    std::cout << "+---------------------+--------------+----------------+" << std::endl;

    for (const auto& result : results) {
        printf("| %-19s | %12.2f | %14d |\n", result.model_name.c_str(), result.avg_time_ms,
               result.faces_detected);
    }

    std::cout << "+---------------------+--------------+----------------+" << std::endl;
    std::cout << "=======================================================\n" << std::endl;
}
