#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.enhancer;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.mocks.foundation.mock_inference_session;
import tests.helpers.foundation.test_utilities;

using namespace domain::face::enhancer;
using namespace foundation::ai::inference_session;
using namespace tests::mocks::foundation;
using namespace tests::helpers::foundation;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class CodeFormerTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance()->clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(CodeFormerTest, LoadModelAndEnhanceFace) {
    std::string model_path = "codeformer.onnx";
    InferenceSessionRegistry::get_instance()->preload_session(model_path, Options(), mock_session);

    auto enhancer = create_enhancer(EnhancerType::CodeFormer);

    // 1. Setup Mock for load_model
    // Input Dims: [1, 3, 512, 512]
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 512, 512}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));
    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));

    Options options;
    enhancer->load_model(model_path, options);

    // 2. Setup Mock for run
    // CodeFormer expects 2 inputs: "input" (image) and "weight" (double)
    // Output: [1, 3, 512, 512]

    int h = 512;
    int w = 512;
    std::vector<int64_t> output_shape = {1, 3, h, w};
    size_t output_size = 1 * 3 * h * w;
    std::vector<float> output_data(output_size, 0.0f);
    // Set some value to check if normalization works back
    // enhance_face logic:
    // val = (val + 1.0) * 127.5
    // If we want result ~127, val should be 0.0
    // If we want result ~255, val should be 1.0

    // Let's set center pixel to 1.0f (so result should be 255)
    int center_idx = (h / 2) * w + (w / 2);
    // Fill all channels at center
    output_data[0 * (h * w) + center_idx] = 1.0f; // R (output is RGB planar)
    output_data[1 * (h * w) + center_idx] = 1.0f; // G
    output_data[2 * (h * w) + center_idx] = 1.0f; // B

    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([&](const std::vector<Ort::Value>&) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;
        outputs.push_back(Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(output_data.data()), output_data.size(),
            const_cast<int64_t*>(output_shape.data()), output_shape.size()));
        return outputs;
    });

    // 3. Execute
    cv::Mat crop = cv::Mat::zeros(512, 512, CV_8UC3);
    auto result = enhancer->enhance_face(crop);

    // 4. Verify
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.rows, 512);
    EXPECT_EQ(result.cols, 512);

    // Check center pixel
    cv::Vec3b pixel = result.at<cv::Vec3b>(256, 256);
    EXPECT_NEAR(pixel[0], 255, 1); // B
    EXPECT_NEAR(pixel[1], 255, 1); // G
    EXPECT_NEAR(pixel[2], 255, 1); // R
}
