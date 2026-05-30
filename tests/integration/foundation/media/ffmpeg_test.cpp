1 | /**
  2| * @file ffmpeg_tests.cpp
  3| * @brief Integration tests for FFmpeg media utilities.
  4| * @author
  5| * CodingRookie
  6| * @date 2026-01-27
  7| */
    8 | 9 | #include < gtest / gtest.h > 10 | #include < gmock / gmock.h > 11
    | #include < filesystem > 12 | #include < string > 13 | #include < opencv2 / core / mat.hpp > 14
    | #include < opencv2 / core.hpp > 15 | 16 | #include < opencv2 / imgproc.hpp > 17 | 18
    | import foundation.media.ffmpeg;
19 | import foundation.infrastructure.file_system;
20 | import tests.helpers.foundation.test_utilities;
21 | 22 | namespace fs = std::filesystem;
23 | using namespace foundation::media::ffmpeg;
24 | using namespace tests::helpers::foundation;
25 | 26 | class FfmpegTest : public ::testing::Test {
    27 | protected : 28 | void SetUp() override {
        29 | // Setup if needed
            30 |
    }
    31 |
};
32 | 33 | TEST_F(FfmpegTest, IsVideoNonExistent) {
    34 | EXPECT_FALSE(is_video("non_existent_video.mp4"));
    35 |
}
36 | 37 | TEST_F(FfmpegTest, IsVideoValid) {
    38 | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    39 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found: " << video_path;
    }
    40 | EXPECT_TRUE(is_video(video_path.string()));
    41 |
}
42 | 43 | TEST_F(FfmpegTest, VideoParamsValid) {
    44 | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    45 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found: " << video_path;
    }
    46 | VideoParams params(video_path.string());
    47 | EXPECT_GT(params.width, 0);
    48 | EXPECT_GT(params.height, 0);
    49 | 50 | EXPECT_GT(params.frameRate, 0);
    51 |
}
52 | 53 | TEST_F(FfmpegTest, ExtractFrames) {
    54 | // No need to check for ffmpeg command, we link directly.
        55 | 56
        | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    57 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found: " << video_path;
    }
    58 | 59 | auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_extract";
    60 | if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    61 | fs::create_directories(temp_dir);
    62 | 63 | // Using %d format for C++ implementation (snprintf)
        64 | std::string pattern = (temp_dir / "frame_%d.jpg").string();
    65 | extract_frames(video_path.string(), pattern);
    66 | 67 | bool found_any = false;
    68 | for (const auto& entry : fs::directory_iterator(temp_dir)) {
        69 | if (entry.path().extension() == ".jpg") {
            70 | found_any = true;
            71 | break;
            72 |
        }
        73 |
    }
    74 | EXPECT_TRUE(found_any);
    75 | 76 | fs::remove_all(temp_dir);
    77 |
}
78 | 79 | TEST_F(FfmpegTest, VideoReaderMetadata) {
    80 | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    81 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found";
    }
    82 | 83 | VideoReader reader(video_path.string());
    84 | ASSERT_TRUE(reader.open());
    85 | 86 | EXPECT_GT(reader.get_width(), 0);
    87 | EXPECT_GT(reader.get_height(), 0);
    88 | EXPECT_GT(reader.get_fps(), 0.0);
    89 | EXPECT_GT(reader.get_frame_count(), 0);
    90 | EXPECT_GT(reader.get_duration_ms(), 0);
    91 |
}
92 | 93 | TEST_F(FfmpegTest, VideoReaderSequentialRead) {
    94 | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    95 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found";
    }
    96 | 97 | VideoReader reader(video_path.string());
    98 | ASSERT_TRUE(reader.open());
    99 | 100 | int count = 0;
    101 | while (true) {
        102 | cv::Mat frame = reader.read_frame();
        103 | if (frame.empty()) break;
        104 | EXPECT_EQ(frame.cols, reader.get_width());
        105 | EXPECT_EQ(frame.rows, reader.get_height());
        106 | count++;
        107 |
    }
    108 | // slideshow_scaled.mp4 is short, just ensure we read something
        109 | EXPECT_GT(count, 0);
    110 | // Allow small deviation in frame count vs metadata estimation
        111 | EXPECT_NEAR(count, reader.get_frame_count(), 5);
    112 |
}
113 | 114 | TEST_F(FfmpegTest, VideoReaderPreciseSeek) {
    115 | auto video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
    116 | if (!fs::exists(video_path)) {
        GTEST_SKIP() << "Test video not found";
    }
    117 | 118 | VideoReader reader(video_path.string());
    119 | ASSERT_TRUE(reader.open());
    120 | 121 | int total_frames = reader.get_frame_count();
    122 | if (total_frames < 10) {
        GTEST_SKIP() << "Video too short for seek test";
    }
    123 | 124 | // Test Case 1: Seek to middle
        125 | int target_idx_1 = total_frames / 2;
    126 | EXPECT_TRUE(reader.seek(target_idx_1));
    127 | 128 | // Read frame at target (should be cached or decoded precisely)
        129 | cv::Mat frame_1 = reader.read_frame();
    130 | EXPECT_FALSE(frame_1.empty());
    131 | 132 | // Calculate approximate timestamp from index
        133 | double expected_ts_1 = target_idx_1 * 1000.0 / reader.get_fps();
    134 | // Allow 2 frame duration error (decoding jitter)
        135
        | EXPECT_NEAR(reader.get_current_timestamp_ms(), expected_ts_1, 2000.0 / reader.get_fps());
    136 | 137 | // Test Case 2: Seek backwards
        138 | int target_idx_2 = 1;
    139 | EXPECT_TRUE(reader.seek(target_idx_2));
    140 | cv::Mat frame_2 = reader.read_frame();
    141 | EXPECT_FALSE(frame_2.empty());
    142 | 143 |                          // Test Case 3: Seek by time
        144 | double target_ms = 1000.0; // 1 second
    145 | if (reader.get_duration_ms() > 1500) {
        146 | EXPECT_TRUE(reader.seek_by_time(target_ms));
        147 | cv::Mat frame_3 = reader.read_frame();
        148 | EXPECT_FALSE(frame_3.empty());
        149 | EXPECT_NEAR(reader.get_current_timestamp_ms(), target_ms, 100.0); // 100ms tolerance
        150 |
    }
    151 |
}
152 | 153 | TEST_F(FfmpegTest, VideoWriterBasicWrite) {
    154 | auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_basic_write";
    155 | if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    156 | fs::create_directories(temp_dir);
    157 | 158 | fs::path output_path = temp_dir / "output_basic.mp4";
    159 | 160 | VideoParams params("");
    161 | params.width = 640;
    162 | params.height = 480;
    163 | params.frameRate = 30;
    164 | params.quality = 18;
    165 | params.videoCodec = "mpeg4";
    166 | 167 | VideoWriter writer(output_path.string(), params);
    168 | ASSERT_TRUE(writer.open());
    169 | EXPECT_TRUE(writer.is_opened());
    170 | 171 | // Generate some frames
        172 | cv::Mat frame(480, 640, CV_8UC3);
    173 | for (int i = 0; i < 30; ++i) {
        174 | frame.setTo(cv::Scalar(i * 5, 0, 0)); // Blue gradient
        175
            | cv::putText(frame, std::to_string(i), cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX,
                          1.0, 176 | cv::Scalar(255, 255, 255), 2);
        177 | EXPECT_TRUE(writer.write_frame(frame));
        178 |
    }
    179 | 180 | EXPECT_EQ(writer.get_written_frame_count(), 30);
    181 | writer.close();
    182 | EXPECT_FALSE(writer.is_opened());
    183 | 184 | EXPECT_TRUE(fs::exists(output_path));
    185 | EXPECT_GT(fs::file_size(output_path), 1024);
    186 | 187 | // Verify output with VideoReader
        188 | {
        189 | VideoReader reader(output_path.string());
        190 | ASSERT_TRUE(reader.open());
        191 | EXPECT_EQ(reader.get_width(), 640);
        192 | EXPECT_EQ(reader.get_height(), 480);
        193 | // Allow small fp error
            194 | EXPECT_NEAR(reader.get_fps(), 30.0, 2.0);
        195 | 196 | // Check we can read frames back
            197 | int read_count = 0;
        198 | while (!reader.read_frame().empty()) {
            read_count++;
        }
        199 | // MPEG4 encoding might drop the last frame or merge it, allow 1 frame loss
            200 | EXPECT_GE(read_count, 29);
        201 | EXPECT_LE(read_count, 30);
        202 |
    }
    203 | 204 | fs::remove_all(temp_dir);
    205 |
}
206 | 207 | TEST_F(FfmpegTest, VideoWriterAdvancedParams) {
    208 | auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_advanced";
    209 | if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    210 | fs::create_directories(temp_dir);
    211 | 212 | std::string output_path = (temp_dir / "high_bitrate.mp4").string();
    213 | 214 | VideoParams params;
    215 | params.width = 640;
    216 | params.height = 480;
    217 | params.frameRate = 30.0;
    218 | 219 |                         // Test Bitrate Control
        220 | params.bitRate = 5000000; // 5 Mbps
    221 | params.maxBitRate = 6000000;
    222 | params.bufSize = 10000000; // Buffer size for VBV
    223 | params.gopSize = 30;       // 1s GOP
    224 | params.pixelFormat = "yuv420p";
    225 | 226 | VideoWriter writer(output_path, params);
    227 | ASSERT_TRUE(writer.open());
    228 | 229 | // Write 2 seconds of synthetic video
        230 | cv::Mat frame(480, 640, CV_8UC3);
    231 | for (int i = 0; i < 60; ++i) {
        232 | frame.setTo(cv::Scalar(i % 255, (i * 2) % 255, (i * 3) % 255));
        233 | // Add some noise to make compression harder
            234 | cv::randu(frame, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255));
        235 | EXPECT_TRUE(writer.write_frame(frame));
        236 |
    }
    237 | writer.close();
    238 | 239 | EXPECT_TRUE(fs::exists(output_path));
    240 | 241 | // Verify file is valid
        242 | {
        243 | VideoReader reader(output_path);
        244 | EXPECT_TRUE(reader.open());
        245 | EXPECT_EQ(reader.get_width(), 640);
        246 | EXPECT_EQ(reader.get_height(), 480);
        247 | // Frame count might vary slightly due to container overhead or B-frames at end
            248 | EXPECT_NEAR(reader.get_frame_count(), 60, 2);
        249 |
    } // reader destroyed here, file closed
    250 | 251 | fs::remove_all(temp_dir);
    252 |
}
253 | 254 | TEST_F(FfmpegTest, ComposeVideoFromImages) {
    255 | auto img1 = get_test_data_path("standard_face_test_images/lenna.bmp");
    256 | if (!fs::exists(img1)) {
        GTEST_SKIP() << "Test image not found: " << img1;
    }
    257 | 258 | auto temp_dir = fs::temp_directory_path() / "facefusion_ffmpeg_test_i2v";
    259 | if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    260 | fs::create_directories(temp_dir);
    261 | 262 | // Copy images to create a sequence
        263 | fs::copy_file(img1, temp_dir / "img_001.bmp");
    264 | fs::copy_file(img1, temp_dir / "img_002.bmp"); // Duplicate is fine for test
    265 | 266 | std::string input_pattern = (temp_dir / "img_%03d.bmp").string();
    267 | std::string output_video = (temp_dir / "output.mp4").string();
    268 | 269 | VideoParams params("");
    270 | // params("") should produce default initialized struct or initialized from empty (no
          // file)
        271 | params.width = 512;
    272 | params.height = 512;
    273 | params.frameRate = 30.0;
    274 | params.quality = 18; // Standard good quality
    275 | params.videoCodec = "libx264";
    276 | params.preset = "ultrafast";
    277 | 278 | bool success = compose_video_from_images(input_pattern, output_video, params);
    279 | EXPECT_TRUE(success);
    280 | EXPECT_TRUE(fs::exists(output_video));
    281 | EXPECT_TRUE(is_video(output_video));
    282 | 283 | fs::remove_all(temp_dir);
    284 |
}
285 |