#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>

import foundation.media.vision;

using namespace foundation::media::vision;

TEST(VisionTest, UnpackResolution) {
    auto size = unpack_resolution("1920x1080");
    EXPECT_EQ(size.width, 1920);
    EXPECT_EQ(size.height, 1080);

    EXPECT_THROW(unpack_resolution("invalid"), std::invalid_argument);
    // unpack_resolution implementation uses stringstream extraction.
    // "100x" might extract width=100 and fail on height.
    EXPECT_THROW(unpack_resolution("100x"), std::invalid_argument);
}

TEST(VisionTest, RestrictResolution) {
    cv::Size s1(100, 100);
    cv::Size s2(50, 50);

    // restrict_resolution returns the one with smaller area
    auto res = restrict_resolution(s1, s2);
    EXPECT_EQ(res.width, 50);
    EXPECT_EQ(res.height, 50);

    auto res2 = restrict_resolution(s2, s1);
    EXPECT_EQ(res2.width, 50);
    EXPECT_EQ(res2.height, 50);
}

TEST(VisionTest, CreateTileFramesSimple) {
    cv::Mat image = cv::Mat::zeros(100, 100, CV_8UC3);
    // tile_size: [tile_size, padding_around_image, overlap/padding_inside]
    // Based on previous analysis:
    // size[0] = tile size
    // size[1] = padding around image
    // size[2] = overlap/border

    std::vector<int> tile_size = {50, 0, 0};

    auto [tiles, pad_w, pad_h] = create_tile_frames(image, tile_size);

    // Input 100x100.
    // Tile width: 50.
    // The implementation adds padding such that pad_size = size[2] + tile_width - (rows %
    // tile_width) For 100%50 == 0, pad = 0 + 50 = 50. Total size becomes 150x150. Loops run for 0,
    // 50, 100. Total 3x3 = 9 tiles.

    EXPECT_EQ(tiles.size(), 9);
    EXPECT_EQ(tiles[0].rows, 50);
    EXPECT_EQ(tiles[0].cols, 50);
}

TEST(VisionTest, ResizeFrame) {
    cv::Mat image = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::Size crop_size(50, 50);

    cv::Mat resized = resize_frame(image, crop_size);
    EXPECT_EQ(resized.rows, 50);
    EXPECT_EQ(resized.cols, 50);

    // If crop size is larger, it should return clone (no upscale)
    cv::Size large_size(200, 200);
    cv::Mat same = resize_frame(image, large_size);
    EXPECT_EQ(same.rows, 100);
    EXPECT_EQ(same.cols, 100);
}
