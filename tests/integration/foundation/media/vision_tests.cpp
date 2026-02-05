/**
 * @file vision_tests.cpp
 * @brief Unit tests for vision utilities.
 * @author CodingRookie
 *
 * @date 2026-01-27
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>
#include <unordered_set>

import foundation.media.vision;
import foundation.infrastructure.file_system;
import foundation.infrastructure.test_support;

namespace fs = std::filesystem;
using namespace foundation::media::vision;
using namespace foundation::infrastructure::test;

class VisionTest : public ::testing::Test {
protected:
    void SetUp() override {
        const testing::TestInfo* const test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        test_dir = std::string("test_vision_sandbox_") + test_info->test_suite_name() + "_"
                 + test_info->name();

        if (fs::exists(test_dir)) { fs::remove_all(test_dir); }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            // Cleanup logic if needed
            fs::remove_all(test_dir);
        }
    }

    std::string test_dir;

    std::string create_dummy_image(const std::string& filename, int width = 64, int height = 64) {
        std::string path = (fs::path(test_dir) / filename).string();
        cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);
        cv::bitwise_not(img, img); // White image
        cv::imwrite(path, img);
        return path;
    }

    std::string create_text_file(const std::string& filename) {
        std::string path = (fs::path(test_dir) / filename).string();
        std::ofstream ofs(path);
        ofs << "not an image";
        ofs.close();
        return path;
    }
};

TEST_F(VisionTest, IsImage) {
    std::string imgPath = create_dummy_image("test.png");
    EXPECT_TRUE(is_image(imgPath));

    std::string txtPath = create_text_file("test.txt");
    EXPECT_FALSE(is_image(txtPath));

    EXPECT_FALSE(is_image("non_existent_file.png"));
}

TEST_F(VisionTest, ReadStaticImage) {
    std::string imgPath = create_dummy_image("read_test.jpg", 100, 50);
    cv::Mat img = read_static_image(imgPath);
    EXPECT_FALSE(img.empty());
    EXPECT_EQ(img.cols, 100);
    EXPECT_EQ(img.rows, 50);
}

TEST_F(VisionTest, ResizeFrame) {
    cv::Mat src = cv::Mat::zeros(100, 100, CV_8UC3);
    cv::Size targetSize(50, 50);
    cv::Mat dst = resize_frame(src, targetSize);
    EXPECT_EQ(dst.cols, 50);
    EXPECT_EQ(dst.rows, 50);

    // If target is larger than source, it should not upscale (returns clone).
    cv::Size largeSize(200, 200);
    cv::Mat dst2 = resize_frame(src, largeSize);
    EXPECT_EQ(dst2.cols, 100);
    EXPECT_EQ(dst2.rows, 100);
}

TEST_F(VisionTest, ReadRealImageLenna) {
    try {
        auto path = get_test_data_path("standard_face_test_images/lenna.bmp");
        if (fs::exists(path)) {
            cv::Mat img = read_static_image(path.string());
            ASSERT_FALSE(img.empty());
            // Lenna is typically 512x512
            EXPECT_GT(img.cols, 0);
            EXPECT_GT(img.rows, 0);
            EXPECT_EQ(img.channels(), 3);
        } else {
            // If assets missing, fail.
            FAIL() << "Test asset not found: " << path.string();
        }
    } catch (const std::exception& e) { FAIL() << "Exception finding asset: " << e.what(); }
}
