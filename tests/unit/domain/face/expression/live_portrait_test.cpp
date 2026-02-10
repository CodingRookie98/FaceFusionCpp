#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.expression;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.mocks.foundation.mock_inference_session;
import tests.helpers.foundation.test_utilities;

using namespace domain::face::expression;
using namespace foundation::ai::inference_session;
using namespace tests::mocks::foundation;
using namespace tests::helpers::foundation;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class LivePortraitTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance()->clear();
        feature_mock = std::make_shared<NiceMock<MockInferenceSession>>();
        motion_mock = std::make_shared<NiceMock<MockInferenceSession>>();
        generator_mock = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> feature_mock;
    std::shared_ptr<NiceMock<MockInferenceSession>> motion_mock;
    std::shared_ptr<NiceMock<MockInferenceSession>> generator_mock;
};

TEST_F(LivePortraitTest, LoadModelAndRestoreExpression) {
    std::string feature_path = "feature_extractor.onnx";
    std::string motion_path = "motion_extractor.onnx";
    std::string generator_path = "generator.onnx";

    InferenceSessionRegistry::get_instance()->preload_session(feature_path, Options(), feature_mock);
    InferenceSessionRegistry::get_instance()->preload_session(motion_path, Options(), motion_mock);
    InferenceSessionRegistry::get_instance()->preload_session(generator_path, Options(),
                                                             generator_mock);

    auto restorer = create_live_portrait_restorer();

    // 1. Setup Feature Mock
    // Input Dims: [1, 3, 256, 256]
    std::vector<std::vector<int64_t>> feature_input_dims = {{1, 3, 256, 256}};
    EXPECT_CALL(*feature_mock, get_input_node_dims()).WillRepeatedly(Return(feature_input_dims));
    EXPECT_CALL(*feature_mock, is_model_loaded()).WillRepeatedly(Return(true));

    // Feature Output: [1, 32, 16, 64, 64] -> size = 2097152
    size_t feature_size = 1 * 32 * 16 * 64 * 64;
    std::vector<int64_t> feature_output_shape = {1, 32, 16, 64, 64};
    std::vector<float> feature_data(feature_size, 0.1f);

    EXPECT_CALL(*feature_mock, run(_)).WillRepeatedly([=](const std::vector<Ort::Value>&) mutable {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;
        outputs.push_back(Ort::Value::CreateTensor<float>(
            memory_info, feature_data.data(), feature_data.size(), feature_output_shape.data(),
            feature_output_shape.size()));
        return outputs;
    });

    // 2. Setup Motion Mock
    // Input Dims: [1, 3, 256, 256]
    std::vector<std::vector<int64_t>> motion_input_dims = {{1, 3, 256, 256}};
    EXPECT_CALL(*motion_mock, get_input_node_dims()).WillRepeatedly(Return(motion_input_dims));
    EXPECT_CALL(*motion_mock, is_model_loaded()).WillRepeatedly(Return(true));
    EXPECT_CALL(*motion_mock, get_output_names())
        .WillRepeatedly(Return(std::vector<std::string>{"0", "1", "2", "3", "4", "5", "6"}));

    // Motion Outputs (7 outputs)
    // 0-3: Scalars (1)
    // 4: Vec3 (3)
    // 5, 6: Vec63 (21*3 = 63)

    EXPECT_CALL(*motion_mock, run(_)).WillRepeatedly([=](const std::vector<Ort::Value>&) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;

        // 0-3: Scalars
        std::vector<int64_t> scalar_shape = {1};
        static float scalar_val = 0.0f; // pitch, yaw, roll, scale
        outputs.push_back(
            Ort::Value::CreateTensor<float>(memory_info, &scalar_val, 1, scalar_shape.data(), 1));
        outputs.push_back(
            Ort::Value::CreateTensor<float>(memory_info, &scalar_val, 1, scalar_shape.data(), 1));
        outputs.push_back(
            Ort::Value::CreateTensor<float>(memory_info, &scalar_val, 1, scalar_shape.data(), 1));
        static float scale_val = 1.0f;
        outputs.push_back(
            Ort::Value::CreateTensor<float>(memory_info, &scale_val, 1, scalar_shape.data(), 1));

        // 4: Translation [3] (Wait, code expects pointer to 3 floats)
        std::vector<int64_t> trans_shape = {1, 3};
        static std::vector<float> trans_val(3, 0.0f);
        outputs.push_back(Ort::Value::CreateTensor<float>(memory_info, trans_val.data(), 3,
                                                          trans_shape.data(), 2));

        // 5: Expression [21, 3]
        std::vector<int64_t> expr_shape = {1, 21, 3};
        static std::vector<float> expr_val(63, 0.0f);
        outputs.push_back(Ort::Value::CreateTensor<float>(memory_info, expr_val.data(), 63,
                                                          expr_shape.data(), 3));

        // 6: Points [21, 3]
        static std::vector<float> pts_val(63, 0.0f);
        outputs.push_back(
            Ort::Value::CreateTensor<float>(memory_info, pts_val.data(), 63, expr_shape.data(), 3));

        return outputs;
    });

    // 3. Setup Generator Mock
    // Output Dims: [1, 3, 512, 512]
    std::vector<std::vector<int64_t>> generator_output_dims = {{1, 3, 512, 512}};
    EXPECT_CALL(*generator_mock, get_output_node_dims())
        .WillRepeatedly(Return(generator_output_dims));
    EXPECT_CALL(*generator_mock, get_input_names())
        .WillRepeatedly(Return(std::vector<std::string>{"feature_volume", "source", "target"}));
    EXPECT_CALL(*generator_mock, is_model_loaded()).WillRepeatedly(Return(true));

    size_t gen_size = 1 * 3 * 512 * 512;
    std::vector<int64_t> gen_shape = {1, 3, 512, 512};
    std::vector<float> gen_data(gen_size, 0.5f); // Grey image

    EXPECT_CALL(*generator_mock, run(_)).WillRepeatedly([=](const std::vector<Ort::Value>&) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;
        outputs.push_back(Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(gen_data.data()), gen_data.size(),
            const_cast<int64_t*>(gen_shape.data()), gen_shape.size()));
        return outputs;
    });

    Options options;
    restorer->load_model(feature_path, motion_path, generator_path, options);

    cv::Mat source = cv::Mat::zeros(256, 256, CV_8UC3);
    cv::Mat target = cv::Mat::zeros(256, 256, CV_8UC3);

    auto result = restorer->restore_expression(source, target, 0.5f);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.cols, 512);
    EXPECT_EQ(result.rows, 512);
}
