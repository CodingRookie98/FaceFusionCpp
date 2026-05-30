#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <onnx/onnx_pb.h>
#include <onnxruntime_cxx_api.h>
#include <fstream>
#include <filesystem>
#include <opencv2/core.hpp>

import domain.face.swapper;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.mocks.foundation.mock_inference_session;
import tests.helpers.foundation.test_utilities;

using namespace domain::face::swapper;
using namespace foundation::ai::inference_session;
using namespace tests::mocks::foundation;
using namespace tests::helpers::foundation;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class InSwapperTest : public ::testing::Test {
protected:
    std::string model_path = "dummy_inswapper.onnx";
    std::shared_ptr<MockInferenceSession> mock_session;

    void SetUp() override {
        // Create dummy ONNX file
        onnx::ModelProto model;
        auto* graph = model.mutable_graph();
        auto* initializer = graph->add_initializer();
        initializer->set_name("arcface_embedding");
        initializer->add_dims(512);
        initializer->add_dims(512);
        initializer->set_data_type(onnx::TensorProto_DataType_FLOAT);
        // Add dummy data
        std::vector<float> data(512 * 512, 0.01f);
        initializer->set_raw_data(
            std::string(reinterpret_cast<const char*>(data.data()), data.size() * sizeof(float)));

        std::fstream output(model_path, std::ios::out | std::ios::trunc | std::ios::binary);
        if (!model.SerializeToOstream(&output)) {
            // Handle error if needed
        }
        output.close();

        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
        InferenceSessionRegistry::get_instance()->preload_session(model_path, Options(),
                                                                  mock_session);
    }

    void TearDown() override {
        InferenceSessionRegistry::get_instance()->clear();
        if (std::filesystem::exists(model_path)) { std::filesystem::remove(model_path); }
    }
};

TEST_F(InSwapperTest, LoadModelAndSwapFace) {
    InSwapper swapper;
    Options options;

    // Expectations
    // EXPECT_CALL(*mock_session, load_model(model_path, _)).Times(1); // Registry returns mock
    // directly, skipping load_model
    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mock_session, get_loaded_model_path()).WillRepeatedly(Return(model_path));

    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 128, 128}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));

    std::vector<std::string> input_names = {"source", "target"};
    EXPECT_CALL(*mock_session, get_input_names()).WillRepeatedly(Return(input_names));

    // Mock run output
    std::vector<int64_t> output_shape = {1, 3, 128, 128};
    size_t output_size = 1 * 3 * 128 * 128;
    std::vector<float> output_data(output_size, 0.5f);

    EXPECT_CALL(*mock_session, run(_)).WillOnce([&](const std::vector<Ort::Value>&) {
        auto mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outs;
        outs.push_back(Ort::Value::CreateTensor<float>(mem, output_data.data(), output_size,
                                                       output_shape.data(), output_shape.size()));
        return outs;
    });

    EXPECT_NO_THROW(swapper.load_model(model_path, options));

    cv::Mat target_img = cv::Mat::zeros(128, 128, CV_8UC3);
    std::vector<float> source_embedding(512, 0.1f);

    cv::Mat result = swapper.swap_face(target_img, source_embedding);

    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.rows, 128);
    EXPECT_EQ(result.cols, 128);
    EXPECT_EQ(result.type(), CV_8UC3);
}

TEST_F(InSwapperTest, SwapFaceThrowsIfNotLoaded) {
    InSwapper swapper;
    cv::Mat target_img = cv::Mat::zeros(128, 128, CV_8UC3);
    std::vector<float> source_embedding(512, 0.1f);

    EXPECT_THROW(swapper.swap_face(target_img, source_embedding), std::runtime_error);
}
