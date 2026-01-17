#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <chrono>

import domain.pipeline;
// import domain.pipeline:api; // Removed partition imports
// import domain.pipeline:adapters;
// import domain.pipeline:impl;
// import domain.pipeline:types;

import domain.face; // Added domain.face
import domain.face.swapper;
import domain.face.detector;
import domain.face.recognizer;
import domain.ai.model_repository;
import foundation.ai.inference_session;
import foundation.infrastructure.test_support;

using namespace domain::pipeline;
using namespace foundation::infrastructure::test;

class PipelineIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo = domain::ai::model_repository::ModelRepository::get_instance();
        auto assets_path = get_assets_path();
        auto models_info_path = assets_path / "models_info.json";
        if (std::filesystem::exists(models_info_path)) {
            repo->set_model_info_file_path(models_info_path.string());
        }

        source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
        output_path = "tests_output/pipeline_output.mp4";

        std::filesystem::create_directories("tests_output");
        if (std::filesystem::exists(output_path)) { std::filesystem::remove(output_path); }
    }

    // Helper to get source embedding
    std::vector<float> get_source_embedding() {
        cv::Mat source_img = cv::imread(source_path.string());
        if (source_img.empty()) return {};

        // Detect
        auto detector = domain::face::detector::FaceDetectorFactory::create(
            domain::face::detector::DetectorType::Yolo);
        std::string det_model = repo->ensure_model("face_detector_yoloface");
        detector->load_model(det_model,
                             foundation::ai::inference_session::Options::with_best_providers());
        auto results = detector->detect(source_img);
        if (results.empty()) return {};

        // Recognize
        auto recognizer = domain::face::recognizer::create_face_recognizer(
            domain::face::recognizer::FaceRecognizerType::ArcFace_w600k_r50);
        std::string rec_model = repo->ensure_model("face_recognizer_arcface_w600k_r50");
        recognizer->load_model(rec_model,
                               foundation::ai::inference_session::Options::with_best_providers());

        return recognizer->recognize(source_img, results[0].landmarks).second;
    }

    std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    std::filesystem::path source_path;
    std::filesystem::path video_path;
    std::filesystem::path output_path;
};

TEST_F(PipelineIntegrationTest, VideoProcessingThroughput) {
    if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        GTEST_SKIP() << "Test assets not found";
    }

    // 1. Prepare Models and Data
    auto source_embedding = get_source_embedding();
    ASSERT_FALSE(source_embedding.empty()) << "Could not extract source embedding";

    auto swapper = domain::face::swapper::FaceSwapperFactory::create_inswapper();
    std::string swap_model = repo->ensure_model("inswapper_128");
    ASSERT_FALSE(swap_model.empty()) << "Swapper model not found";
    swapper->load_model(swap_model,
                        foundation::ai::inference_session::Options::with_best_providers());

    // 2. Setup Pipeline
    PipelineConfig config;
    config.worker_thread_count = 2; // Use 2 workers for test
    config.max_queue_size = 16;
    config.max_concurrent_gpu_tasks = 2;

    auto pipeline = std::make_shared<Pipeline>(config);
    // pipeline->add_processor(std::make_shared<SwapperAdapter>(swapper)); // Manually added later

    class TestDetectorProcessor : public IFrameProcessor {
    public:
        TestDetectorProcessor(std::shared_ptr<domain::face::detector::IFaceDetector> d) :
            detector(d) {}
        void process(FrameData& frame) override {
            auto results = detector->detect(frame.image);
            if (!results.empty()) {
                // Pass landmarks to metadata for Swapper
                domain::face::swapper::SwapInput input;
                input.target_frame = frame.image;
                input.target_faces_landmarks = {results[0].landmarks}; // Swap first face

                // We also need source embedding.
                if (frame.metadata.contains("source_embedding")) {
                    input.source_embedding =
                        std::any_cast<std::vector<float>>(frame.metadata.at("source_embedding"));
                }

                frame.metadata["swap_input"] = input;
            }
        }
        std::shared_ptr<domain::face::detector::IFaceDetector> detector;
    };

    auto detector = domain::face::detector::FaceDetectorFactory::create(
        domain::face::detector::DetectorType::Yolo);
    std::string det_model = repo->ensure_model("face_detector_yoloface");
    detector->load_model(det_model,
                         foundation::ai::inference_session::Options::with_best_providers());

    // Convert unique_ptr to shared_ptr explicitly
    std::shared_ptr<domain::face::detector::IFaceDetector> shared_detector = std::move(detector);

    // Add Detector then Swapper
    pipeline->add_processor(std::make_shared<TestDetectorProcessor>(shared_detector));
    pipeline->add_processor(std::make_shared<SwapperAdapter>(swapper));

    pipeline->start();

    {
        // 3. Start Producer
        std::jthread producer([&]() {
            cv::VideoCapture cap(video_path.string());
            if (!cap.isOpened()) return;

            int frame_count = 0;
            int max_frames = 30; // Limit frames for test speed

            cv::Mat frame;
            while (frame_count < max_frames && cap.read(frame)) {
                FrameData data;
                data.sequence_id = frame_count++;
                data.timestamp_ms = cap.get(cv::CAP_PROP_POS_MSEC);
                data.image = frame.clone();

                // Inject source embedding into metadata so DetectorProcessor can use it
                data.metadata["source_embedding"] = source_embedding;

                pipeline->push_frame(std::move(data));
            }

            // Push EOS
            FrameData eos;
            eos.is_end_of_stream = true;
            eos.sequence_id = frame_count;
            pipeline->push_frame(std::move(eos));
        });

        // 4. Start Consumer
        std::atomic<int> processed_count = 0;
        std::jthread consumer([&]() {
            cv::VideoWriter writer;
            // Delayed open until we get first frame to know size

            while (true) {
                auto data_opt = pipeline->pop_frame();
                if (!data_opt) break; // Shutdown

                FrameData& data = *data_opt;
                if (data.is_end_of_stream) { break; }

                if (!writer.isOpened()) {
                    // Try mp4v for better compatibility in test environments
                    writer.open(output_path.string(), cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
                                30.0, data.image.size());
                }

                if (writer.isOpened()) {
                    writer.write(data.image);
                } else {
                    std::cerr << "Failed to open video writer for " << output_path << std::endl;
                }
                processed_count++;
            }
            writer.release();
        });
    } // Threads join here

    // 5. Verification
    EXPECT_TRUE(std::filesystem::exists(output_path)) << "Output video file should exist";
    if (std::filesystem::exists(output_path)) {
        EXPECT_GT(std::filesystem::file_size(output_path), 1024)
            << "Output video should not be empty";

        // Verify valid video
        cv::VideoCapture cap_out(output_path.string());
        EXPECT_TRUE(cap_out.isOpened()) << "Should be able to open output video";
        if (cap_out.isOpened()) {
            cv::Mat frame;
            EXPECT_TRUE(cap_out.read(frame)) << "Should have at least one frame";
        }
        cap_out.release();
    }
}

class MockProcessor : public IFrameProcessor {
public:
    void process(FrameData& frame) override {
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Mark frame as processed
        frame.metadata["processed"] = true;
    }
};

TEST(PipelineSchedulerTest, SchedulerLogic) {
    PipelineConfig config;
    config.worker_thread_count = 4;
    config.max_queue_size = 10;

    auto pipeline = std::make_shared<Pipeline>(config);
    pipeline->add_processor(std::make_shared<MockProcessor>());
    pipeline->start();

    const int frame_count = 20;

    // Producer
    std::jthread producer([&] {
        for (int i = 0; i < frame_count; ++i) {
            FrameData data;
            data.sequence_id = i;
            data.image = cv::Mat::zeros(100, 100, CV_8UC3);
            pipeline->push_frame(std::move(data));
        }
        FrameData eos;
        eos.is_end_of_stream = true;
        eos.sequence_id = frame_count;
        pipeline->push_frame(std::move(eos));
    });

    // Consumer
    int consumed_count = 0;
    std::vector<long long> sequence_ids;

    while (true) {
        auto data_opt = pipeline->pop_frame();
        if (!data_opt) break;

        if (data_opt->is_end_of_stream) break;

        EXPECT_TRUE(data_opt->metadata.contains("processed"));
        sequence_ids.push_back(data_opt->sequence_id);
        consumed_count++;
    }

    EXPECT_EQ(consumed_count, frame_count);

    // Verify all frames received.
    // Check strict order
    for (size_t i = 0; i < sequence_ids.size(); ++i) {
        EXPECT_EQ(sequence_ids[i], i) << "Frame order mismatch at index " << i;
    }
}
