#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <opencv2/core.hpp>
import tests.common.generators.image_generator;
import tests.common.generators.face_generator;

TEST(GeneratorsTest, CreateBlackImage) {
    auto img = tests::common::generators::create_black_image(100, 100);
    EXPECT_EQ(img.rows, 100);
    EXPECT_EQ(img.cols, 100);
    EXPECT_EQ(img.type(), CV_8UC3);
    // Check if truly black (sum of all pixels is 0)
    EXPECT_EQ(cv::sum(img)[0], 0);
}

TEST(GeneratorsTest, CreateTestFace) {
    auto face = tests::common::generators::create_valid_face();
    EXPECT_FALSE(face.kps().empty());
    EXPECT_FALSE(face.embedding().empty());
}
