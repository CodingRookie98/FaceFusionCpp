#include <gtest/gtest.h>
#include <vector>
#include <string>

import services.pipeline.utils;

using namespace services::pipeline;

TEST(TargetSortingTest, SortsImagesAndVideosCorrectly) {
    std::vector<std::string> inputs = {"img1.jpg", "vid1.mp4", "img2.png", "vid2.avi", "img3.bmp"};

    auto is_video_mock = [](const std::string& path) {
        return path.find(".mp4") != std::string::npos || path.find(".avi") != std::string::npos;
    };

    auto result = sort_targets_by_type(inputs, is_video_mock);

    ASSERT_EQ(result.images.size(), 3);
    EXPECT_EQ(result.images[0], "img1.jpg");
    EXPECT_EQ(result.images[1], "img2.png");
    EXPECT_EQ(result.images[2], "img3.bmp");

    ASSERT_EQ(result.videos.size(), 2);
    EXPECT_EQ(result.videos[0], "vid1.mp4");
    EXPECT_EQ(result.videos[1], "vid2.avi");
}

TEST(TargetSortingTest, HandlesEmptyInput) {
    std::vector<std::string> inputs;
    auto is_video_mock = [](const std::string&) { return false; };
    auto result = sort_targets_by_type(inputs, is_video_mock);
    EXPECT_TRUE(result.images.empty());
    EXPECT_TRUE(result.videos.empty());
}

TEST(TargetSortingTest, PreservesOrderWithinCategories) {
    std::vector<std::string> inputs = {"v1.mp4", "i1.jpg", "v2.mp4", "i2.jpg"};
    auto is_video_mock = [](const std::string& s) { return s.find(".mp4") != std::string::npos; };

    auto result = sort_targets_by_type(inputs, is_video_mock);

    EXPECT_EQ(result.images.size(), 2);
    EXPECT_EQ(result.images[0], "i1.jpg");
    EXPECT_EQ(result.images[1], "i2.jpg");

    EXPECT_EQ(result.videos.size(), 2);
    EXPECT_EQ(result.videos[0], "v1.mp4");
    EXPECT_EQ(result.videos[1], "v2.mp4");
}
