#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

import foundation.media.ffmpeg;
import foundation.infrastructure.file_system;
import foundation.infrastructure.test_support;

namespace fs = std::filesystem;
using namespace foundation::media::ffmpeg;
using namespace foundation::infrastructure::test;

class FfmpegStreamTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup paths
        video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
        output_dir = fs::temp_directory_path() / "facefusion_ffmpeg_stream_test";
        if (fs::exists(output_dir)) fs::remove_all(output_dir);
        fs::create_directories(output_dir);
    }

    void TearDown() override {
        if (fs::exists(output_dir)) fs::remove_all(output_dir);
    }

    fs::path video_path;
    fs::path output_dir;
};

TEST_F(FfmpegStreamTest, VideoReader_OpenAndRead) {
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }

    VideoReader reader(video_path.string());
    EXPECT_TRUE(reader.open());
    EXPECT_TRUE(reader.is_opened());

    EXPECT_GT(reader.get_width(), 0);
    EXPECT_GT(reader.get_height(), 0);
    EXPECT_GT(reader.get_fps(), 0.0);
    EXPECT_GT(reader.get_frame_count(), 0);
    EXPECT_GT(reader.get_duration_ms(), 0);

    int frames_read = 0;
    while (true) {
        cv::Mat frame = reader.read_frame();
        if (frame.empty()) break;

        EXPECT_EQ(frame.cols, reader.get_width());
        EXPECT_EQ(frame.rows, reader.get_height());
        EXPECT_EQ(frame.type(), CV_8UC3); // BGR
        frames_read++;
    }

    // Allow small variance in frame count calculation vs actual read
    EXPECT_NEAR(frames_read, reader.get_frame_count(), 5);
    reader.close();
    EXPECT_FALSE(reader.is_opened());
}

TEST_F(FfmpegStreamTest, VideoReader_Seek) {
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }

    VideoReader reader(video_path.string());
    ASSERT_TRUE(reader.open());

    int mid_frame = reader.get_frame_count() / 2;
    EXPECT_TRUE(reader.seek(mid_frame));

    cv::Mat frame = reader.read_frame();
    EXPECT_FALSE(frame.empty());

    // Approximate check - exact timestamp matching depends on GOP structure
    double expected_ts = mid_frame * 1000.0 / reader.get_fps();
    double actual_ts = reader.get_current_timestamp_ms();

    // Allow 500ms deviation due to seeking to keyframes
    EXPECT_NEAR(actual_ts, expected_ts, 500.0);
}

TEST_F(FfmpegStreamTest, VideoWriter_WriteVideo) {
    fs::path output_path = output_dir / "output.mp4";

    VideoPrams params("");
    params.width = 640;
    params.height = 480;
    params.frameRate = 30;
    params.quality = 18;
    params.videoCodec = "mpeg4";

    VideoWriter writer(output_path.string(), params);
    ASSERT_TRUE(writer.open());
    EXPECT_TRUE(writer.is_opened());

    // Generate some frames
    cv::Mat frame(480, 640, CV_8UC3);
    for (int i = 0; i < 30; ++i) {
        frame.setTo(cv::Scalar(i * 5, 0, 0)); // Blue gradient
        cv::putText(frame, std::to_string(i), cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0,
                    cv::Scalar(255, 255, 255), 2);
        EXPECT_TRUE(writer.write_frame(frame));
    }

    EXPECT_EQ(writer.get_written_frame_count(), 30);
    writer.close();
    EXPECT_FALSE(writer.is_opened());

    EXPECT_TRUE(fs::exists(output_path));
    EXPECT_GT(fs::file_size(output_path), 1024);

    // Verify output with VideoReader
    VideoReader reader(output_path.string());
    ASSERT_TRUE(reader.open());
    EXPECT_EQ(reader.get_width(), 640);
    EXPECT_EQ(reader.get_height(), 480);
    // Allow small fp error
    EXPECT_NEAR(reader.get_fps(), 30.0, 2.0);

    // Check we can read frames back
    int read_count = 0;
    while (!reader.read_frame().empty()) { read_count++; }
    // MPEG4 encoding might drop the last frame or merge it, allow 1 frame loss
    EXPECT_GE(read_count, 29);
    EXPECT_LE(read_count, 30);
}
