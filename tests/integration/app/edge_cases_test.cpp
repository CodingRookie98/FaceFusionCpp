1 | #include < gtest / gtest.h > 2 | #include < filesystem > 3 | #include < opencv2 / opencv.hpp > 4
    | #include < regex > 5 | #include < fstream > 6 | #include < memory > 7 | 8
    | import services.pipeline.runner;
9 | import config.task;
10 | import config.app;
11 | import config.merger;
12 | import tests.helpers.foundation.test_utilities;
13 | import foundation.infrastructure.logger;
14 | import foundation.media.vision;
15 | import foundation.media.ffmpeg;
16 | import domain.ai.model_repository;
17 | 18 | using namespace services::pipeline;
19 | using namespace tests::helpers::foundation;
20 | 21 | extern void LinkGlobalTestEnvironment();
22 | 23 | class EdgeCasesTest : public ::testing::Test {
    24 | protected : 25 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    26 | 27 | void SetUp() override {
        28 | repo_ = domain::ai::model_repository::ModelRepository::get_instance();
        29 | auto assets_path = get_assets_path();
        30 | repo_->set_model_info_file_path((assets_path / "models_info.json").string());
        31 | 32 | source_path_ = assets_path / "standard_face_test_images" / "lenna.bmp";
        33 | 34 | output_dir_ =
            std::filesystem::temp_directory_path() / "facefusion_tests" / "edge_cases";
        35 | std::filesystem::create_directories(output_dir_);
        36 |
    }
    37 | 38 | void TearDown() override {
        39 | if (std::filesystem::exists(output_dir_)) {
            40 | std::error_code ec;
            41 | std::filesystem::remove_all(output_dir_, ec);
            42 |
        }
        43 |
    }
    44 | 45 | std::filesystem::path source_path_;
    46 | std::filesystem::path output_dir_;
    47 | std::shared_ptr<domain::ai::model_repository::ModelRepository> repo_;
    48 |
};
49 | 50 | // ============================================================================
    51 |  // Edge Case 1: Palette image (pal8) auto-conversion
    52 |  // ============================================================================
    53 | 54 | TEST_F(EdgeCasesTest, PaletteImageAutoConvertsToRGB24) {
    55 | auto target_path = get_assets_path() / "standard_face_test_images" / "man.bmp";
    56 | auto output_path = output_dir_ / "result_man.bmp";
    57 | 58 | cv::Mat input = cv::imread(target_path.string(), cv::IMREAD_UNCHANGED);
    59 | ASSERT_FALSE(input.empty()) << "Failed to load man.bmp";
    60 | 61 | config::TaskConfig task_config;
    62 | task_config.task_info.id = "palette_edge_test";
    63 | task_config.io.source_paths = {source_path_.string()};
    64 | task_config.io.target_paths = {target_path.string()};
    65 | task_config.io.output.path = output_dir_.string();
    66 | task_config.io.output.image_format = "bmp";
    67 | 68 | config::PipelineStep swap_step;
    69 | swap_step.step = "face_swapper";
    70 | swap_step.enabled = true;
    71 | config::FaceSwapperParams params;
    72 | params.model = "inswapper_128_fp16";
    73 | swap_step.params = params;
    74 | task_config.pipeline.push_back(swap_step);
    75 | 76 | config::AppConfig app_config;
    77 | 78 | auto runner = create_pipeline_runner(app_config);
    79 | auto merged_config = config::MergeConfigs(task_config, app_config);
    80 | auto result = runner->run(merged_config);
    81 | 82 | if (!result.is_ok()) {
        83 | std::cout << "Pipeline failed with error: " << result.error().message << std::endl;
        84 |
    }
    85 | ASSERT_TRUE(result.is_ok()) << "Pipeline should handle pal8 format";
    86 | ASSERT_TRUE(std::filesystem::exists(output_path)) << "Output should be generated";
    87 | 88 | cv::Mat output = cv::imread(output_path.string());
    89 | EXPECT_EQ(output.channels(), 3) << "Output should be RGB (3 channels)";
    90 | EXPECT_EQ(output.type(), CV_8UC3) << "Output should be 8-bit BGR";
    91 |
}
92 | 93 | // ============================================================================
    94 |  // Edge Case 2: Format disguise (WebP with .jpg extension)
    95 |  // ============================================================================
    96 | 97 | TEST_F(EdgeCasesTest, FormatDisguiseWebPWithJpgExtensionDecodesCorrectly) {
    98 | auto target_path = get_assets_path() / "standard_face_test_images" / "woman.jpg";
    99 | auto output_path = output_dir_ / "result_woman.png";
    100 | 101 | {
        102 | std::ifstream file(target_path, std::ios::binary);
        103 | char magic[12];
        104 | file.read(magic, 12);
        105 | bool is_webp =
            (std::string(magic, 4) == "RIFF" && std::string(magic + 8, 4) == "WEBP");
        106 | EXPECT_TRUE(is_webp) << "woman.jpg should actually be WebP format";
        107 |
    }
    108 | 109 | config::TaskConfig task_config;
    110 | task_config.task_info.id = "format_disguise_test";
    111 | task_config.io.source_paths = {source_path_.string()};
    112 | task_config.io.target_paths = {target_path.string()};
    113 | task_config.io.output.path = output_dir_.string();
    114 | task_config.io.output.image_format = "png";
    115 | 116 | config::PipelineStep swap_step;
    117 | swap_step.step = "face_swapper";
    118 | swap_step.enabled = true;
    119 | config::FaceSwapperParams params;
    120 | params.model = "inswapper_128_fp16";
    121 | swap_step.params = params;
    122 | task_config.pipeline.push_back(swap_step);
    123 | 124 | config::AppConfig app_config;
    125 | 126 | auto runner = create_pipeline_runner(app_config);
    127 | auto merged_config = config::MergeConfigs(task_config, app_config);
    128 | auto result = runner->run(merged_config);
    129 | 130 | if (!result.is_ok()) {
        131 | std::cout << "Pipeline failed with error: " << result.error().message << std::endl;
        132 |
    }
    133 | ASSERT_TRUE(result.is_ok()) << "Pipeline should handle WebP disguised as JPG";
    134 | ASSERT_TRUE(std::filesystem::exists(output_path));
    135 | 136 | cv::Mat output = cv::imread(output_path.string());
    137 | EXPECT_FALSE(output.empty()) << "Output image should be valid";
    138 |
}
139 | 140 | // ============================================================================
    141 |   // Edge Case 3: No-face frame passthrough (E403)
    142 |   // ============================================================================
    143 | 144 | class NoFaceFrameTest : public EdgeCasesTest {
    145 | protected : 146 | cv::Mat create_no_face_image(int width = 640, int height = 480) {
        147 | cv::Mat img(height, width, CV_8UC3, cv::Scalar(100, 150, 200));
        148 | cv::circle(img, cv::Point(width / 2, height / 2), 100, cv::Scalar(255, 0, 0), -1);
        149 | cv::rectangle(img, cv::Point(50, 50), cv::Point(150, 150), cv::Scalar(0, 255, 0), -1);
        150 | return img;
        151 |
    }
    152 |
};
153 | 154 | TEST_F(NoFaceFrameTest, NoFaceDetectedPassthroughWithWarning) {
    155 | auto no_face_img = create_no_face_image();
    156 | auto target_path = output_dir_ / "no_face_input.bmp";
    157 | cv::imwrite(target_path.string(), no_face_img);
    158 | 159 | auto output_path = output_dir_ / "result_no_face_input.bmp";
    160 | 161 | config::TaskConfig task_config;
    162 | task_config.task_info.id = "no_face_test";
    163 | task_config.io.source_paths = {source_path_.string()};
    164 | task_config.io.target_paths = {target_path.string()};
    165 | task_config.io.output.path = output_dir_.string();
    166 | task_config.io.output.image_format = "bmp";
    167 | 168 | config::PipelineStep swap_step;
    169 | swap_step.step = "face_swapper";
    170 | swap_step.enabled = true;
    171 | config::FaceSwapperParams params;
    172 | params.model = "inswapper_128_fp16";
    173 | swap_step.params = params;
    174 | task_config.pipeline.push_back(swap_step);
    175 | 176 | config::AppConfig app_config;
    177 | 178 | auto runner = create_pipeline_runner(app_config);
    179 | auto merged_config = config::MergeConfigs(task_config, app_config);
    180 | auto result = runner->run(merged_config);
    181 | 182 | EXPECT_TRUE(result.is_ok()) << "Pipeline should not fail on no-face images";
    183 | EXPECT_TRUE(std::filesystem::exists(output_path)) << "Output should exist (passthrough)";
    184 | 185 | cv::Mat output = cv::imread(output_path.string());
    186 | EXPECT_FALSE(output.empty());
    187 |
}
188 | 189 | // ============================================================================
    190 |   // Edge Case 4: Vertical video aspect ratio preservation
    191 |   // ============================================================================
    192 | 193 | TEST_F(EdgeCasesTest, VerticalVideoPreservesAspectRatio) {
    194 | auto target_path =
        get_assets_path() / "standard_face_test_videos" / "slideshow_scaled.mp4";
    195 | 196 | foundation::media::ffmpeg::VideoParams video_params(target_path.string());
    197 | int orig_width = video_params.width;
    198 | int orig_height = video_params.height;
    199 | double orig_aspect = static_cast<double>(orig_width) / orig_height;
    200 | 201 | ASSERT_LT(orig_aspect, 1.0) << "Test video should be vertical (portrait)";
    202 | ASSERT_EQ(orig_width, 720);
    203 | ASSERT_EQ(orig_height, 1280);
    204 | 205 | auto output_path = output_dir_ / "result_slideshow_scaled.mp4";
    206 | 207 | config::TaskConfig task_config;
    208 | task_config.task_info.id = "vertical_video_test";
    209 | task_config.io.source_paths = {source_path_.string()};
    210 | task_config.io.target_paths = {target_path.string()};
    211 | task_config.io.output.path = output_dir_.string();
    212 | task_config.io.output.image_format = "png";
    213 | 214 | config::PipelineStep swap_step;
    215 | swap_step.step = "face_swapper";
    216 | swap_step.enabled = true;
    217 | config::FaceSwapperParams swap_params;
    218 | swap_params.model = "inswapper_128_fp16";
    219 | swap_step.params = swap_params;
    220 | task_config.pipeline.push_back(swap_step);
    221 | 222 | config::AppConfig app_config;
    223 | 224 | auto runner = create_pipeline_runner(app_config);
    225 | auto merged_config = config::MergeConfigs(task_config, app_config);
    226 | auto result = runner->run(merged_config);
    227 | 228 | ASSERT_TRUE(result.is_ok());
    229 | ASSERT_TRUE(std::filesystem::exists(output_path));
    230 | 231 | foundation::media::ffmpeg::VideoParams out_params(output_path.string());
    232 | int out_width = out_params.width;
    233 | int out_height = out_params.height;
    234 | double out_aspect = static_cast<double>(out_width) / out_height;
    235 | 236 | EXPECT_EQ(out_width, orig_width) << "Width should match";
    237 | EXPECT_EQ(out_height, orig_height) << "Height should match";
    238 | EXPECT_NEAR(out_aspect, orig_aspect, 0.01) << "Aspect ratio should be preserved";
    239 |
}
240 |