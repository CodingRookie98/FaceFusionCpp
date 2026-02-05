#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.masker;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::face::masker;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::Return;
using ::testing::_;
using ::testing::NiceMock;

class OcclusionMaskerTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance().clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(OcclusionMaskerTest, LoadModelAndCreateMask) {
    std::string model_path = "face_occluder.onnx";
    InferenceSessionRegistry::get_instance().preload_session(model_path, Options(), mock_session);

    // 1. Setup Mock for load_model (called inside create_occlusion_masker)
    std::vector<std::vector<int64_t>> input_dims = {{1, 256, 256, 3}};
    EXPECT_CALL(*mock_session, get_input_node_dims())
        .WillRepeatedly(Return(input_dims));
    
    EXPECT_CALL(*mock_session, is_model_loaded())
        .WillRepeatedly(Return(true));

    Options options;
    auto masker = create_occlusion_masker(model_path, options);

    // 2. Setup Mock for run
    cv::Mat frame = cv::Mat::zeros(512, 512, CV_8UC3);

    // Output: [1, 256, 256, 1] (Mask)
    int h = 256;
    int w = 256;
    std::vector<int64_t> output_shape = {1, h, w, 1};
    size_t output_size = h * w;
    std::vector<float> output_data(output_size, 1.0f); // All ones -> Masked

    // Construct Ort::Value
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> output_tensors;
    output_tensors.push_back(Ort::Value::CreateTensor<float>(
        memory_info, output_data.data(), output_data.size(), output_shape.data(), output_shape.size()));

    EXPECT_CALL(*mock_session, run(_))
        .WillOnce(Return(std::move(output_tensors)));

    // 3. Execute
    cv::Mat mask = masker->create_occlusion_mask(frame);

    // 4. Verify
    EXPECT_FALSE(mask.empty());
    EXPECT_EQ(mask.rows, 512);
    EXPECT_EQ(mask.cols, 512);
    // Since output was all 1.0f, and logic thresholds at 0.5, result should be 255.
    // However, resize and blur might affect edges. Center should be 255.
    EXPECT_EQ(mask.at<uint8_t>(256, 256), 255);
}
