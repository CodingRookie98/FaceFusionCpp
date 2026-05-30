1 | #include < gtest / gtest.h > 2 | #include < gmock / gmock.h > 3
    | #include < opencv2 / core.hpp > 4 | import tests.mocks.foundation.mock_inference_session;
5 | import tests.mocks.domain.mock_face_detector;
6 | import tests.mocks.domain.mock_face_enhancer;
7 | import tests.mocks.domain.mock_model_repository;
8 | 9 | using namespace tests::mocks::foundation;
10 | using namespace tests::mocks::domain;
11 | using ::testing::_;
12 | using ::testing::Return;
13 | 14 | TEST(MockUsageTest, MockInferenceSessionCanBeInstantiated) {
    15 | MockInferenceSession session;
    16 | EXPECT_CALL(session, is_model_loaded()).WillOnce(Return(true));
    17 | EXPECT_TRUE(session.is_model_loaded());
    18 |
}
19 | 20 | TEST(MockUsageTest, MockFaceDetectorCanBeInstantiated) {
    21 | MockFaceDetector detector;
    22 | // Just verify instantiation and basic mock functionality
        23 | EXPECT_CALL(detector, load_model(_, _)).Times(1);
    24 | detector.load_model("path", {});
    25 |
}
26 | 27 | TEST(MockUsageTest, MockFaceEnhancerCanBeInstantiated) {
    28 | MockFaceEnhancer enhancer;
    29 | cv::Mat dummy = cv::Mat::zeros(10, 10, CV_8UC3);
    30 | EXPECT_CALL(enhancer, enhance_face(_)).WillOnce(Return(dummy));
    31 | cv::Mat result = enhancer.enhance_face(dummy);
    32 | EXPECT_EQ(result.rows, 10);
    33 |
}
34 | 35 | TEST(MockUsageTest, MockModelRepositoryCanBeInstantiated) {
    36 | MockModelRepository repo;
    37 | EXPECT_CALL(repo, ensure_model("test_model")).WillOnce(Return("/path/to/model"));
    38 | EXPECT_EQ(repo.ensure_model("test_model"), "/path/to/model");
    39 |
}
40 |