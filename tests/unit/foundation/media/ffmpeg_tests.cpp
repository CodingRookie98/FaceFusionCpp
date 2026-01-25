#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <string>
#include <opencv2/core/mat.hpp>
#include <opencv2/core.hpp>

import foundation.media.ffmpeg;
import foundation.infrastructure.file_system;
import foundation.infrastructure.test_support;

namespace fs = std::filesystem;
using namespace foundation::media::ffmpeg;
using namespace foundation::infrastructure::test;

class FfmpegTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
};

TEST_F(FfmpegTest, IsVideoNonExistent) {
    EXPECT_FALSE(is_video("non_existent_video.mp4"));
}

TEST_F(FfmpegTest, IsVideoValid) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }
    EXPECT_TRUE(is_video(video_path.string()));
}

TEST_F(FfmpegTest, VideoParamsValid) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }
    VideoParams params(video_path.string());
    EXPECT_GT(params.width, 0);
    EXPECT_GT(params.height, 0);

    EXPECT_GT(params.frameRate, 0);
}

TEST_F(FfmpegTest, ExtractFrames) {
    // No need to check for ffmpeg command, we link directly.

    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }

    auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_extract";
    if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

    // Using %d format for C++ implementation (snprintf)
    std::string pattern = (temp_dir / "frame_%d.jpg").string();
    extract_frames(video_path.string(), pattern);

    bool found_any = false;
    for (const auto& entry : fs::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".jpg") {
            found_any = true;
            break;
        }
    }
    EXPECT_TRUE(found_any);

    fs::remove_all(temp_dir);
}

TEST_F(FfmpegTest, VideoReader_Metadata) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found"; }

    VideoReader reader(video_path.string());
    ASSERT_TRUE(reader.open());

    EXPECT_GT(reader.get_width(), 0);
    EXPECT_GT(reader.get_height(), 0);
    EXPECT_GT(reader.get_fps(), 0.0);
    EXPECT_GT(reader.get_frame_count(), 0);
    EXPECT_GT(reader.get_duration_ms(), 0);
}

TEST_F(FfmpegTest, VideoReader_SequentialRead) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found"; }

    VideoReader reader(video_path.string());
    ASSERT_TRUE(reader.open());

    int count = 0;
    while (true) {
        cv::Mat frame = reader.read_frame();
        if (frame.empty()) break;
        EXPECT_EQ(frame.cols, reader.get_width());
        EXPECT_EQ(frame.rows, reader.get_height());
        count++;
    }
    // slideshow_scaled.mp4 is short, just ensure we read something
    EXPECT_GT(count, 0);
    // Allow small deviation in frame count vs metadata estimation
    EXPECT_NEAR(count, reader.get_frame_count(), 5);
}

TEST_F(FfmpegTest, VideoReader_PreciseSeek) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found"; }

    VideoReader reader(video_path.string());
    ASSERT_TRUE(reader.open());

    int total_frames = reader.get_frame_count();
    if (total_frames < 10) { GTEST_SKIP() << "Video too short for seek test"; }

    // Test Case 1: Seek to middle
    int target_idx_1 = total_frames / 2;
    EXPECT_TRUE(reader.seek(target_idx_1));

    // Read frame at target (should be cached or decoded precisely)
    cv::Mat frame_1 = reader.read_frame();
    EXPECT_FALSE(frame_1.empty());

    // Calculate approximate timestamp from index
    double expected_ts_1 = target_idx_1 * 1000.0 / reader.get_fps();
    // Allow 2 frame duration error (decoding jitter)
    EXPECT_NEAR(reader.get_current_timestamp_ms(), expected_ts_1, 2000.0 / reader.get_fps());

    // Test Case 2: Seek backwards
    int target_idx_2 = 1;
    EXPECT_TRUE(reader.seek(target_idx_2));
    cv::Mat frame_2 = reader.read_frame();
    EXPECT_FALSE(frame_2.empty());

    // Test Case 3: Seek by time
    double target_ms = 1000.0; // 1 second
    if (reader.get_duration_ms() > 1500) {
        EXPECT_TRUE(reader.seek_by_time(target_ms));
        cv::Mat frame_3 = reader.read_frame();
        EXPECT_FALSE(frame_3.empty());
        EXPECT_NEAR(reader.get_current_timestamp_ms(), target_ms, 100.0); // 100ms tolerance
    }
}

TEST_F(FfmpegTest, VideoWriter_AdvancedParams) {
    auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_advanced";
    if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

    std::string output_path = (temp_dir / "high_bitrate.mp4").string();

    VideoParams params;
    params.width = 640;
    params.height = 480;
    params.frameRate = 30.0;

    // Test Bitrate Control
    params.bitRate = 5000000; // 5 Mbps
    params.maxBitRate = 6000000;
    params.bufSize = 10000000; // Buffer size for VBV
    params.gopSize = 30;       // 1s GOP
    params.pixelFormat = "yuv420p";

    VideoWriter writer(output_path, params);
    ASSERT_TRUE(writer.open());

    // Write 2 seconds of synthetic video
    cv::Mat frame(480, 640, CV_8UC3);
    for (int i = 0; i < 60; ++i) {
        frame.setTo(cv::Scalar(i % 255, (i * 2) % 255, (i * 3) % 255));
        // Add some noise to make compression harder
        cv::randu(frame, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
        EXPECT_TRUE(writer.write_frame(frame));
    }
    writer.close();

    EXPECT_TRUE(fs::exists(output_path));

    // Verify file is valid
    {
        VideoReader reader(output_path);
        EXPECT_TRUE(reader.open());
        EXPECT_EQ(reader.get_width(), 640);
        EXPECT_EQ(reader.get_height(), 480);
        // Frame count might vary slightly due to container overhead or B-frames at end
        EXPECT_NEAR(reader.get_frame_count(), 60, 2);
    } // reader destroyed here, file closed

    fs::remove_all(temp_dir);
}

TEST_F(FfmpegTest, ComposeVideoFromImages) {
    auto img1 = get_test_data_path("standard_face_test_images/lenna.bmp");
    if (!fs::exists(img1)) { GTEST_SKIP() << "Test image not found: " << img1; }

    auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_i2v";
    if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

    // Copy images to create a sequence
    fs::copy_file(img1, temp_dir / "img_001.bmp");
    fs::copy_file(img1, temp_dir / "img_002.bmp"); // Duplicate is fine for test

    std::string input_pattern = (temp_dir / "img_%03d.bmp").string();
    std::string output_video = (temp_dir / "output.mp4").string();

    VideoParams params("");
    // params("") should produce default initialized struct or initialized from empty (no file)
    params.width = 512;
    params.height = 512;
    params.frameRate = 30.0;
    params.quality = 18; // Standard good quality
    params.videoCodec = "libx264";
    params.preset = "ultrafast";

    bool success = compose_video_from_images(input_pattern, output_video, params);
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(output_video));
    EXPECT_TRUE(is_video(output_video));

    fs::remove_all(temp_dir);
}
