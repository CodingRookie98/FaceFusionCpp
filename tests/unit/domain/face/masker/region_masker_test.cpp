#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <onnxruntime_cxx_api.h>

import domain.face.masker;
import foundation.ai.inference_session;
import foundation.ai.inference_session_registry;
import tests.test_support.foundation.ai.mock_inference_session;

using namespace domain::face::masker;
using namespace foundation::ai::inference_session;
using namespace tests::test_support::foundation::ai;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class RegionMaskerTest : public ::testing::Test {
protected:
    void SetUp() override {
        InferenceSessionRegistry::get_instance()->clear();
        mock_session = std::make_shared<NiceMock<MockInferenceSession>>();
    }

    std::shared_ptr<NiceMock<MockInferenceSession>> mock_session;
};

TEST_F(RegionMaskerTest, LoadModelAndCreateMask) {
    std::string model_path = "face_parser.onnx";
    InferenceSessionRegistry::get_instance()->preload_session(model_path, Options(), mock_session);

    Options options;
    auto masker = create_region_masker(model_path, options);

    // 1. Setup Mock for load_model
    // Input Dims: [1, 3, 512, 512]
    std::vector<std::vector<int64_t>> input_dims = {{1, 3, 512, 512}};
    EXPECT_CALL(*mock_session, get_input_node_dims()).WillRepeatedly(Return(input_dims));
    EXPECT_CALL(*mock_session, is_model_loaded()).WillRepeatedly(Return(true));

    // 2. Setup Mock for run
    // Output: [1, NumClasses=19, H=512, W=512]
    // Classes: 0=Back, 1=Skin, ...
    int num_classes = 19;
    int h = 512;
    int w = 512;
    std::vector<int64_t> output_shape = {1, num_classes, h, w};
    size_t output_size = 1 * num_classes * h * w;
    std::vector<float> output_data(output_size, 0.0f);

    // Set a region in the center to be Skin (Class 1)
    // We want the result at (256, 256) to be Skin.
    // The masker flips the output horizontally.
    // Flip(x) = Width - 1 - x = 511 - x.
    // If we want result x=256, we need source x such that 511 - x = 256 => x = 255.
    int center_y = 256;
    int center_x = 255;
    int center_idx = center_y * w + center_x;

    // For flat buffer: [c * pixels + pixel_idx]
    // Class 1 (Skin)
    output_data[1 * (h * w) + center_idx] = 10.0f;
    // Class 0 (Back) stays 0.0f

    // Set a region at top-left to be Background (Class 0)
    int tl_idx = 0;
    output_data[0 * (h * w) + tl_idx] = 10.0f;

    EXPECT_CALL(*mock_session, run(_)).WillRepeatedly([=](const std::vector<Ort::Value>&) {
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        std::vector<Ort::Value> outputs;
        outputs.push_back(Ort::Value::CreateTensor<float>(
            memory_info, const_cast<float*>(output_data.data()), output_data.size(),
            const_cast<int64_t*>(output_shape.data()), output_shape.size()));
        return outputs;
    });

    // 3. Execute
    cv::Mat frame = cv::Mat::zeros(512, 512, CV_8UC3);
    // Request Skin Mask
    std::unordered_set<FaceRegion> regions = {FaceRegion::Skin};
    cv::Mat mask = masker->create_region_mask(frame, regions);

    // 4. Verify
    EXPECT_FALSE(mask.empty());
    EXPECT_EQ(mask.rows, 512);
    EXPECT_EQ(mask.cols, 512);

    // Center should be 255 (Skin)
    // Top-Left should be 0 (Background)
    // Note: The logic flips the mask vertically at the end!
    // cv::flip(mask, mask, 1); // 1 is horizontal flip?
    // Let's check impl.
    // impl_region.cpp: cv::flip(mask, mask, 1);
    // 1 means flip around y-axis (horizontal mirror).
    // Center stays center.
    // Top-Left (0,0) becomes Top-Right (0, 511).

    EXPECT_EQ(mask.at<uint8_t>(256, 256), 255);
    EXPECT_EQ(mask.at<uint8_t>(0, 511), 0); // Flipped TL
}
