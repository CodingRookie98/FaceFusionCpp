1 | #include < gtest / gtest.h > 2 | #include < gmock / gmock.h > 3
    | #include < opencv2 / core.hpp > 4 | import tests.common.matchers.opencv_matchers;
5 | 6 | using namespace tests::common::matchers;
7 | 8 | TEST(OpenCVMatchersTest, MatEqExactMatch) {
    9 | cv::Mat a = cv::Mat::zeros(10, 10, CV_8UC1);
    10 | cv::Mat b = cv::Mat::zeros(10, 10, CV_8UC1);
    11 | EXPECT_THAT(a, MatEq(b));
    12 |
}
13 | 14 | TEST(OpenCVMatchersTest, MatEqMismatch) {
    15 | cv::Mat a = cv::Mat::zeros(10, 10, CV_8UC1);
    16 | cv::Mat b = cv::Mat::ones(10, 10, CV_8UC1);
    17 | EXPECT_THAT(a, ::testing::Not(MatEq(b)));
    18 |
}
19 |