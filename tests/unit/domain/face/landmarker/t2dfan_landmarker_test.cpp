#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.landmarker;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::face::landmarker;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

class T2dfanLandmarkerTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance().clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(T2dfanLandmarkerTest, LoadModelAndDetectLandmarks) {
    std::string model_path = "2dfan4.onnx";
    InferenceSessionRegistry::get_instance().preload_session(model_path, Options(), mock_session);

    auto landmarker = create_landmarker(LandmarkerType::T2dfan);

    // 1. Setup Mock for load_model
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 256, 256}};
    EXPECT_CALL(*mock_session, get_input_node_dims())
        .WillRepeatedly(Return(input_dims));
    
    EXPECT_CALL(*mock_session, is_model_loaded())
        .WillRepeatedly(Return(true));

    Options options;
    landmarker->load_model(model_path, options);

    // 2. Setup Mock for detect (run)
    // Create a dummy input frame
    cv::Mat frame = cv::Mat::zeros(512, 512, CV_8UC3);
    // BBox covering the whole image for simplicity, so no scaling/translation effects 
    // if the logic handles it straightforwardly, but T2dfan usually crops.
    // Let's use a center crop bbox to test transform.
    cv::Rect2f bbox(128, 128, 256, 256);

    // Prepare Output Tensor
    // Shape: [1, 68, 3] -> [Batch, NumPoints, Coords+Score]
    int num_points = 68;
    int num_features = 3;
    std::vector<int64_t> output_shape = {1, num_points, num_features};
    size_t output_size = 1 * num_points * num_features;
    std::vector<float> output_data(output_size, 0.0f);

    // Set point 0 to be at center of input (128, 128 in 256x256)
    // T2dfan logic: x = val / 64 * width. 
    // If width=256, to get 128: 128 = val / 64 * 256 => 128 = val * 4 => val = 32.
    output_data[0] = 32.0f; // x
    output_data[1] = 32.0f; // y
    output_data[2] = 1.0f;  // score

    // Construct Ort::Value
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> output_tensors;
    output_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, output_data.data(), output_data.size(), output_shape.data(), output_shape.size()));

    EXPECT_CALL(*mock_session, run(_))
        .WillOnce(Return(std::move(output_tensors)));

    // 3. Execute
    auto result = landmarker->detect(frame, bbox);

    // 4. Verify
    ASSERT_EQ(result.landmarks.size(), 68);
    // The point (128, 128) in 256x256 crop corresponds to the center of the bbox.
    // BBox is (128, 128, 256, 256). Center is (128+128, 128+128) = (256, 256).
    // So expected point 0 is at (256, 256).
    
    // Note: T2dfan pre-processing might scale/pad differently.
    // The logic in T2dfan::Impl::pre_process:
    // scale = 195 / max(w, h) = 195 / 256 = 0.7617
    // translation logic...
    // This makes exact calculation tricky without replicating the logic.
    // However, if we assume the transformation is correct, we expect the point to be roughly at center of bbox.
    
    EXPECT_NEAR(result.landmarks[0].x, 256.0f, 5.0f);
    EXPECT_NEAR(result.landmarks[0].y, 256.0f, 5.0f);
    EXPECT_GT(result.score, 0.0f);
}
