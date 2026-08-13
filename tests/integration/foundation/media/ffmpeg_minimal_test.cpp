// Minimal repro: write 80 frames via VideoWriter and check decodable frame count
#include <gtest/gtest.h>
#include <filesystem>
#include <opencv2/opencv.hpp>

import foundation.media.ffmpeg;

using namespace foundation::media::ffmpeg;

TEST(VideoWriterMinimalTest, Write80FramesDecodable) {
    std::string out_path = "/tmp/ff_writer_min_80.mp4";
    if (std::filesystem::exists(out_path)) std::filesystem::remove(out_path);

    VideoParams params;
    params.width = 720;
    params.height = 1280;
    params.frameRate = 30.0;
    params.videoCodec = "libx264";
    params.quality = 80;

    VideoWriter writer(out_path, params);
    ASSERT_TRUE(writer.open());

    cv::Mat frame(1280, 720, CV_8UC3, cv::Scalar(120, 80, 40));
    for (int i = 0; i < 80; ++i) {
        ASSERT_TRUE(writer.write_frame(frame)) << "write_frame failed at " << i;
    }
    writer.close();

    ASSERT_TRUE(std::filesystem::exists(out_path));
    std::cout << "[Minimal] written_frame_count=" << writer.get_written_frame_count() << std::endl;

    // Read back and verify decodable frame count
    VideoReader reader(out_path);
    ASSERT_TRUE(reader.open());
    std::cout << "[Minimal] container frame_count=" << reader.get_frame_count() << std::endl;
    int decoded = 0;
    while (!reader.read_frame().empty()) { decoded++; }
    std::cout << "[Minimal] decodable frames=" << decoded << std::endl;
    reader.close();
    EXPECT_EQ(decoded, 80) << "VideoWriter lost frames";
}
