#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <onnxruntime_cxx_api.h>

import domain.face.recognizer;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.mocks.foundation.mock_inference_session;
import tests.helpers.foundation.test_utilities;

using namespace domain::face::recognizer;
using namespace foundation::ai::inference_session;
using namespace tests::mocks::foundation;
using namespace tests::helpers::foundation;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class ArcFaceRecognizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance()->clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(ArcFaceRecognizerTest, LoadModelAndRecognizeFace) {
    std::string model_path = "arcface_w600k_r50.onnx";
    InferenceSessionRegistry::get_instance()->preload_session(model_path, Options(), mock_session);

    auto recognizer = create_face_recognizer(FaceRecognizerType::ArcFaceW600kR50);

    // 1. Setup Mock for load_model
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 112, 112}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));

    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));

    Options options;
    recognizer->load_model(model_path, options);

    // 2. Setup Mock for recognize (run)
    // Create dummy input frame
    cv::Mat frame = cv::Mat::zeros(512, 512, CV_8UC3);
    // Create dummy 5 landmarks
    std::vector<cv::Point2f> kps = {{100, 100}, {200, 100}, {150, 150}, {120, 200}, {180, 200}};

    // Prepare Output Tensor
    // Shape: [1, 512]
    int feature_len = 512;
    std::vector<int64_t> output_shape = {1, feature_len};
    size_t output_size = 1 * feature_len;
    std::vector<float> output_data(output_size, 0.1f); // Set constant value

    // Construct Ort::Value
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> output_tensors;
    output_tensors.push_back(
        Ort::Value::CreateTensor<float>(memory_info, output_data.data(), output_data.size(),
                                        output_shape.data(), output_shape.size()));

    EXPECT_CALL(*mock_session, run(_)).WillOnce(Return(std::move(output_tensors)));

    // 3. Execute
    auto [embedding, normed_embedding] = recognizer->recognize(frame, kps);

    // 4. Verify
    ASSERT_EQ(embedding.size(), 512);
    ASSERT_EQ(normed_embedding.size(), 512);

    // Verify normalization
    // 512 values of 0.1f.
    // Norm L2 = sqrt(512 * 0.1^2) = sqrt(5.12) = 2.2627
    // Normed value = 0.1 / 2.2627 = 0.04419
    float expected_norm_val = 0.1f / std::sqrt(512.0f * 0.1f * 0.1f);
    EXPECT_NEAR(normed_embedding[0], expected_norm_val, 1e-4);

    // Check L2 norm of result is 1.0
    double norm = cv::norm(normed_embedding, cv::NORM_L2);
    EXPECT_NEAR(norm, 1.0, 1e-4);
}
