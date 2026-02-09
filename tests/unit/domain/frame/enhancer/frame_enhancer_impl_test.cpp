#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

import domain.frame.enhancer;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::frame::enhancer;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class FrameEnhancerImplTest : public ::testing::Test {
protected:
    std::string model_path = "dummy_enhancer.onnx";
    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;

    void SetUp() override {
        InferenceSessionRegistry::get_instance()->clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
        InferenceSessionRegistry::get_instance()->preload_session(model_path, Options(),
                                                                 mock_session);

        // Default expectations
        EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));
        EXPECT_CALL(*mock_session, get_input_node_dims())
            .WillRepeatedly(Return(std::vector<std::vector<int64_t>>{{1, 3, -1, -1}}));
    }

    void TearDown() override { InferenceSessionRegistry::get_instance()->clear(); }
};

TEST_F(FrameEnhancerImplTest, EnhanceFrameSimple1x) {
    // Tile size 128, Padding 0, Overlap 0
    std::vector<int> tile_size = {128, 0, 0};
    int model_scale = 1;
    Options options;

    FrameEnhancerImpl enhancer(model_path, options, tile_size, model_scale);

    cv::Mat input_frame = cv::Mat::zeros(128, 128, CV_8UC3);
    FrameEnhancerInput input{input_frame, 100};

    // Output setup
    std::vector<int64_t> output_shape = {1, 3, 128, 128};
    size_t output_size = 1 * 3 * 128 * 128;
    std::vector<float> output_data(output_size, 1.0f);

    // Expect multiple calls due to create_tile_frames implementation details (padding)
    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([&](const std::vector<Ort::Value>&) {
        auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outs;
        outs.push_back(Ort::Value::CreateTensor<float>(mem, output_data.data(), output_size,
                                                       output_shape.data(), output_shape.size()));
        return outs;
    });

    cv::Mat result = enhancer.enhance_frame(input);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.rows, 128);
    EXPECT_EQ(result.cols, 128);
    cv::Vec3b pixel = result.at<cv::Vec3b>(64, 64);
    EXPECT_NEAR(pixel[0], 255, 1);
}

TEST_F(FrameEnhancerImplTest, EnhanceFrameWithScaling2x) {
    // Tile size 64, Padding 0, Overlap 0
    std::vector<int> tile_size = {64, 0, 0};
    int model_scale = 2;
    Options options;

    FrameEnhancerImpl enhancer(model_path, options, tile_size, model_scale);

    // Input 64x64, Output should be 128x128
    cv::Mat input_frame = cv::Mat::zeros(64, 64, CV_8UC3);
    FrameEnhancerInput input{input_frame, 100};

    // Output setup
    int out_h = 64 * model_scale;
    int out_w = 64 * model_scale;
    std::vector<int64_t> output_shape = {1, 3, out_h, out_w};
    size_t output_size = 1 * 3 * out_h * out_w;
    // Need to allocate fresh data for each call if using WillRepeatedly with pointer
    // But since we use lambda, we can just return a vector with CreateTensor.
    // However, CreateTensor doesn't copy data.
    // We must ensure the data pointer is valid.
    // Let's use a static buffer or member variable.
    // Here we use a large enough buffer for repeated use.

    static std::vector<float> static_output_data(output_size, 1.0f);

    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([&](const std::vector<Ort::Value>&) {
        auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outs;
        outs.push_back(Ort::Value::CreateTensor<float>(mem, static_output_data.data(), output_size,
                                                       output_shape.data(), output_shape.size()));
        return outs;
    });

    cv::Mat result = enhancer.enhance_frame(input);

    EXPECT_EQ(result.rows, 128);
    EXPECT_EQ(result.cols, 128);
}

TEST_F(FrameEnhancerImplTest, EnhanceFrameWithTiling) {
    // Tile size 64, Padding 0, Overlap 0
    std::vector<int> tile_size = {64, 0, 0};
    int model_scale = 1;
    Options options;

    FrameEnhancerImpl enhancer(model_path, options, tile_size, model_scale);

    cv::Mat input_frame = cv::Mat::zeros(128, 128, CV_8UC3);
    FrameEnhancerInput input{input_frame, 100};

    // We expect multiple calls to run.
    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([&](const std::vector<Ort::Value>& inputs) {
        auto type_info = inputs[0].GetTensorTypeAndShapeInfo();
        auto shape = type_info.GetShape();
        int h = static_cast<int>(shape[2]);
        int w = static_cast<int>(shape[3]);

        std::vector<int64_t> out_shape = {1, 3, h, w};
        size_t out_size = 1 * 3 * h * w;

        // Use a leak for simplicity in test to ensure valid pointer
        float* data = new float[out_size];
        std::fill_n(data, out_size, 1.0f);

        auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outs;
        outs.push_back(Ort::Value::CreateTensor<float>(mem, data, out_size, out_shape.data(),
                                                       out_shape.size()));
        return outs;
    });

    cv::Mat result = enhancer.enhance_frame(input);

    EXPECT_EQ(result.rows, 128);
    EXPECT_EQ(result.cols, 128);
}
