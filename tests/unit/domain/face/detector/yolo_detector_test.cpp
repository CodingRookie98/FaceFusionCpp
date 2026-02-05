#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.detector;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::face::detector;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class YoloDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear registry
        InferenceSessionRegistry::get_instance().clear();

        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();

        // Register mock session for the expected model path
        // Note: The actual path used in production might differ, but for unit tests
        // we can assume the detector will ask for a specific name or we mock the registry
        // to return this session regardless of path if possible, but the registry
        // is key-based.
        // We assume the test will pass a specific path to load_model.
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(YoloDetectorTest, LoadModelAndDetectFace) {
    std::string model_path = "yoloface_8n.onnx";
    InferenceSessionRegistry::get_instance().preload_session(model_path, Options(), mock_session);

    auto detector = FaceDetectorFactory::create(DetectorType::Yolo);

    // 1. Setup Mock for load_model
    // Yolo calls get_input_node_dims to determine input size
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 640, 640}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));

    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));

    InferenceOptions options;
    detector->load_model(model_path, options);

    // 2. Setup Mock for detect (run)
    // Create a dummy input frame
    cv::Mat frame = cv::Mat::zeros(1280, 1280, CV_8UC3); // 2x scale of 640x640

    // Prepare Output Tensor
    // Shape: [1, 20, 100] -> [Batch, Features, NumBoxes]
    int num_boxes = 100;
    int num_features = 20;
    std::vector<int64_t> output_shape = {1, num_features, num_boxes};
    size_t output_size = 1 * num_features * num_boxes;
    std::vector<float> output_data(output_size, 0.0f);

    // Set up one valid face at index 0
    int box_idx = 0;
    // cx, cy, w, h (normalized to input size 640x640?)
    // Yolo logic:
    // xmin = (cx - 0.5*w) * ratio
    // The model output is likely in absolute coordinates of the input tensor (640x640)
    // or relative? Usually YOLOv8 exports are absolute.
    // Let's assume absolute 640x640 based on typical ONNX exports.

    // Face at center: 320, 320, size 100, 100
    // Index mapping: channel * num_boxes + box_idx
    output_data[0 * num_boxes + box_idx] = 320.0f; // cx
    output_data[1 * num_boxes + box_idx] = 320.0f; // cy
    output_data[2 * num_boxes + box_idx] = 100.0f; // w
    output_data[3 * num_boxes + box_idx] = 100.0f; // h
    output_data[4 * num_boxes + box_idx] = 0.9f;   // score > 0.5

    // Landmarks (5 points)
    // j=5 (x), j=6 (y)
    output_data[5 * num_boxes + box_idx] = 300.0f; // l1.x
    output_data[6 * num_boxes + box_idx] = 300.0f; // l1.y
    // ... others 0.0f

    // Construct Ort::Value
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> output_tensors;
    output_tensors.push_back(
        Ort::Value::CreateTensor<float>(memory_info, output_data.data(), output_data.size(),
                                        output_shape.data(), output_shape.size()));

    EXPECT_CALL(*mock_session, run(_)).WillOnce(Return(std::move(output_tensors)));

    // 3. Execute
    auto results = detector->detect(frame);

    // 4. Verify
    ASSERT_EQ(results.size(), 1);
    auto& face = results[0];
    EXPECT_FLOAT_EQ(face.score, 0.9f);

    // Ratio is 1280/640 = 2.0
    // Expected Box:
    // xmin = (320 - 50) * 2 = 540
    // ymin = (320 - 50) * 2 = 540
    // xmax = (320 + 50) * 2 = 740
    // ymax = (320 + 50) * 2 = 740
    // w = 200, h = 200
    EXPECT_NEAR(face.box.x, 540.0f, 1.0f);
    EXPECT_NEAR(face.box.y, 540.0f, 1.0f);
    EXPECT_NEAR(face.box.width, 200.0f, 1.0f);
    EXPECT_NEAR(face.box.height, 200.0f, 1.0f);

    // Landmark 1
    // 300 * 2 = 600
    EXPECT_NEAR(face.landmarks[0].x, 600.0f, 1.0f);
    EXPECT_NEAR(face.landmarks[0].y, 600.0f, 1.0f);
}
