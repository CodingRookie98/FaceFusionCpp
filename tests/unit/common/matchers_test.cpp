#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
import tests.common.matchers.opencv_matchers;

using namespace tests::common::matchers;

TEST(OpenCVMatchersTest, MatEq_ExactMatch) {
    cv::Mat a = cv::Mat::zeros(10, 10, CV_8UC1);
    cv::Mat b = cv::Mat::zeros(10, 10, CV_8UC1);
    EXPECT_THAT(a, MatEq(b));
}

TEST(OpenCVMatchersTest, MatEq_Mismatch) {
    cv::Mat a = cv::Mat::zeros(10, 10, CV_8UC1);
    cv::Mat b = cv::Mat::ones(10, 10, CV_8UC1);
    EXPECT_THAT(a, ::testing::Not(MatEq(b)));
}
