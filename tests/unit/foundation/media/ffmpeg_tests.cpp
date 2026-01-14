#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <string>

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

TEST_F(FfmpegTest, AudioCodecEnum) {
    EXPECT_EQ(get_audio_codec("aac"), Audio_Codec::Codec_AAC);
    EXPECT_EQ(get_audio_codec("mp3"), Audio_Codec::Codec_MP3);
    EXPECT_EQ(get_audio_codec("unknown_xyz"), Audio_Codec::Codec_UNKNOWN);
}

TEST_F(FfmpegTest, IsVideoNonExistent) {
    EXPECT_FALSE(is_video("non_existent_video.mp4"));
}

TEST_F(FfmpegTest, IsVideoValid) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }
    EXPECT_TRUE(is_video(video_path.string()));
}

TEST_F(FfmpegTest, VideoPramsValid) {
    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }
    VideoPrams params(video_path.string());
    EXPECT_GT(params.width, 0);
    EXPECT_GT(params.height, 0);
    EXPECT_GT(params.frameRate, 0);
}

TEST_F(FfmpegTest, ExtractFrames) {
    // Check if ffmpeg is available
    auto check = child_process("ffmpeg -version");
    bool ffmpeg_found = true;
    for (const auto& line : check) {
        if (line.find("Process exited") != std::string::npos) {
            ffmpeg_found = false;
            break;
        }
    }
    if (!ffmpeg_found) { GTEST_SKIP() << "ffmpeg not found in path"; }

    auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    if (!fs::exists(video_path)) { GTEST_SKIP() << "Test video not found: " << video_path; }

    auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_extract";
    if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

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

TEST_F(FfmpegTest, ImagesToVideo) {
    // Check if ffmpeg is available
    auto check = child_process("ffmpeg -version");
    bool ffmpeg_found = true;
    for (const auto& line : check) {
        if (line.find("Process exited") != std::string::npos) {
            ffmpeg_found = false;
            break;
        }
    }
    if (!ffmpeg_found) { GTEST_SKIP() << "ffmpeg not found in path"; }

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

    VideoPrams params("");
    params.width = 512;
    params.height = 512;
    params.frameRate = 30;
    params.quality = 18; // Standard good quality
    params.videoCodec = "libx264";
    params.preset = "ultrafast";

    bool success = images_to_video(input_pattern, output_video, params);
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(output_video));
    EXPECT_TRUE(is_video(output_video));

    fs::remove_all(temp_dir);
}
