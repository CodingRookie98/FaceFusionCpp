#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
import tests.mocks.foundation.mock_inference_session;
import tests.mocks.domain.mock_face_detector;
import tests.mocks.domain.mock_face_enhancer;
import tests.mocks.domain.mock_model_repository;

using namespace tests::mocks::foundation;
using namespace tests::mocks::domain;
using ::testing::_;
using ::testing::Return;

TEST(MockUsageTest, MockInferenceSession_CanBeInstantiated) {
    MockInferenceSession session;
    EXPECT_CALL(session, is_model_loaded()).WillOnce(Return(true));
    EXPECT_TRUE(session.is_model_loaded());
}

TEST(MockUsageTest, MockFaceDetector_CanBeInstantiated) {
    MockFaceDetector detector;
    // Just verify instantiation and basic mock functionality
    EXPECT_CALL(detector, load_model(_, _)).Times(1);
    detector.load_model("path", {});
}

TEST(MockUsageTest, MockFaceEnhancer_CanBeInstantiated) {
    MockFaceEnhancer enhancer;
    cv::Mat dummy = cv::Mat::zeros(10, 10, CV_8UC3);
    EXPECT_CALL(enhancer, enhance_face(_)).WillOnce(Return(dummy));
    cv::Mat result = enhancer.enhance_face(dummy);
    EXPECT_EQ(result.rows, 10);
}

TEST(MockUsageTest, MockModelRepository_CanBeInstantiated) {
    MockModelRepository repo;
    EXPECT_CALL(repo, ensure_model("test_model")).WillOnce(Return("/path/to/model"));
    EXPECT_EQ(repo.ensure_model("test_model"), "/path/to/model");
}
