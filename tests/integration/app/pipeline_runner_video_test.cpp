1 | /**
  2| * @file pipeline_runner_video_test.cpp
  3| * @brief Integration tests for video processing with PipelineRunner
  4| * @author CodingRookie
  5| * @date 2026-01-27
  6| */
    7 | #include < gtest / gtest.h > 8 | #include < filesystem > 9 | #include < iostream > 10
    | #include < vector > 11 | #include < algorithm > 12 | #include < opencv2 / opencv.hpp > 13 | 14
    | import services.pipeline.runner;
15 | import config.task;
16 | import config.merger;
17 | import domain.ai.model_repository;
18 | import tests.helpers.foundation.test_utilities;
19 | import domain.face;
20 | import domain.face.analyser;
21 | import tests.helpers.domain.face_test_helpers;
22 | import foundation.media.ffmpeg;
23 | 24 | import tests.helpers.foundation.test_constants;
25 | 26 | using namespace services::pipeline;
27 | using namespace tests::helpers::foundation;
28 | using namespace domain::face::analyser;
29 | using namespace std::chrono;
30 | 31 | extern void LinkGlobalTestEnvironment();
32 | 33 | class PipelineRunnerVideoTest : public ::testing::Test {
    34 | protected : 35 | static void SetUpTestSuite() { LinkGlobalTestEnvironment(); }
    36 | 37 | void SetUp() override {
        38 | repo = domain::ai::model_repository::ModelRepository::get_instance();
        39 | auto assets_path = get_assets_path();
        40 | auto models_info_path = assets_path / "models_info.json";
        41 | if (std::filesystem::exists(models_info_path)) {
            42 | repo->set_model_info_file_path(models_info_path.string());
            43 |
        }
        44 | 45 | source_path = get_test_data_path("standard_face_test_images/lenna.bmp");
        46 | video_path = get_test_data_path("standard_face_test_videos/slideshow_scaled.mp4");
        47 | 48 | output_dir =
            49
            | std::filesystem::temp_directory_path() / "facefusion_tests" / "pipeline_runner_video";
        50 | std::filesystem::create_directories(output_dir);
        51 |
    }
    52 | 53 | std::shared_ptr<domain::ai::model_repository::ModelRepository> repo;
    54 | std::filesystem::path source_path;
    55 | std::filesystem::path video_path;
    56 | std::filesystem::path output_dir;
    57 | 58 | struct VideoInfo {
        59 | int frame_count;
        60 | double fps;
        61 | int width;
        62 | int height;
        63 | bool has_audio;
        64 |
    };
    65 | 66 | VideoInfo get_video_info(const std::filesystem::path& video_path) {
        67 | if (!std::filesystem::exists(video_path)) {
            68 | std::cerr << "[ERROR] Video file does not exist: " 69 |
                << std::filesystem::absolute(video_path) << std::endl;
            70 | return {0, 0.0, 0, 0, false};
            71 |
        }
        72 | 73 | try {
            74 | foundation::media::ffmpeg::VideoParams params(video_path.string());
            75 | if (params.width == 0 || params.height == 0) {
                76
                    | std::cerr << "[ERROR] Failed to read video info using ffmpeg module: "
                                << video_path 77
                    | << std::endl;
                78 | return {0, 0.0, 0, 0, false};
                79 |
            }
            80 | 81 | VideoInfo info;
            82 | info.frame_count = static_cast<int>(params.frameCount);
            83 | info.fps = params.frameRate;
            84 | info.width = static_cast<int>(params.width);
            85 | info.height = static_cast<int>(params.height);
            86 | info.has_audio = false;
            87 | 88 | if (info.frame_count <= 0) {
                89 | std::cerr << "[WARN] FFmpeg returned 0 frames, checking via VideoReader..." 90
                    | << std::endl;
                91 | foundation::media::ffmpeg::VideoReader reader(video_path.string());
                92 | if (reader.open()) {
                    info.frame_count = reader.get_frame_count();
                }
                93 |
            }
            94 | 95 | return info;
            96 | 97 |
        } catch (const std::exception& e) {
            98 | std::cerr << "[ERROR] Exception reading video info: " << e.what() << std::endl;
            99 | return {0, 0.0, 0, 0, false};
            100 |
        }
        101 |
    }
    102 | 103
        | void VerifyVideoContent(const std::filesystem::path& video_file,
                                  104 | const std::filesystem::path& source_face_img,
                                  float expected_scale) {
        105 | if (!std::filesystem::exists(video_file)) {
            106 | FAIL() << "Output video file does not exist: " << video_file;
            107 |
        }
        108 | 109 | foundation::media::ffmpeg::VideoReader reader(video_file.string());
        110 | ASSERT_TRUE(reader.open()) << "Failed to open output video";
        111 | 112 | double fps = reader.get_fps();
        113 | int total_frames = reader.get_frame_count();
        114 | int width = reader.get_width();
        115 | int height = reader.get_height();
        116 | 117
            | std::cout << "Verifying video: " << video_file << " [Frames: " << total_frames 118 |
            << ", Size: " << width << "x" << height << ", FPS: " << fps << "]" << std::endl;
        119 | 120 | // 1. Resolution Check
            121 | foundation::media::ffmpeg::VideoReader reader_orig(video_path.string());
        122 | ASSERT_TRUE(reader_orig.open());
        123 | int orig_width = reader_orig.get_width();
        124 | int orig_height = reader_orig.get_height();
        125 | 126
            | EXPECT_NEAR(width, orig_width * expected_scale, 2.0); // Allow slight rounding diff
        127 | EXPECT_NEAR(height, orig_height * expected_scale, 2.0);
        128 | 129 | // 2. Similarity Check (Uniform Sampling)
            130 | auto analyser = tests::helpers::domain::create_face_analyser(repo);
        131 | cv::Mat src_img = cv::imread(source_face_img.string());
        132 | if (src_img.empty()) {
            133
                | std::cout << "Warning: Failed to load source image: " << source_face_img
                            << std::endl;
            134 | return;
            135 |
        }
        136 | 137 | auto source_faces = analyser->get_many_faces(
            138 | src_img, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
        139 | 140 | if (source_faces.empty()) {
            141 | std::cout 142 |
                << "Warning: Could not detect face in source image. Skipping similarity check." 143
                | << std::endl;
            144 | return;
            145 |
        }
        146 | 147 | int valid_frames = 0;
        148 | int passed_frames = 0;
        149 | int frames_to_check = 10;
        150 | int step = std::max(1, total_frames / frames_to_check);
        151 | 152 | for (int i = 0; i < total_frames; i += step) {
            153 | if (!reader.seek(i)) continue;
            154 | cv::Mat frame = reader.read_frame();
            155 | if (frame.empty()) break;
            156 | 157 | auto frame_faces = analyser->get_many_faces(
                158 | frame, FaceAnalysisType::Detection | FaceAnalysisType::Embedding);
            159 | 160 | if (!frame_faces.empty()) {
                161 | valid_frames++;
                162 | // Check closest face to source
                    163 | float min_dist = 100.0f;
                164 | for (const auto& face : frame_faces) {
                    165 | float dist = FaceAnalyser::calculate_face_distance(source_faces[0], face);
                    166 | if (dist < min_dist) min_dist = dist;
                    167 |
                }
                168 | 169 | if (min_dist < 0.65f) {
                    170 | passed_frames++;
                    171 |
                }
                else {
                    172
                        | std::cout << "Frame " << i
                                    << " failed similarity check. Dist: " << min_dist 173
                        | << std::endl;
                    174 |
                }
                175 |
            }
            176 |
        }
        177 | 178 | std::cout << "Similarity Check: " << passed_frames << "/" << valid_frames 179 |
            << " frames passed." << std::endl;
        180 | 181 | if (valid_frames > 0) {
            182 | double pass_rate = static_cast<double>(passed_frames) / valid_frames;
            183 | EXPECT_GE(pass_rate, tests::helpers::foundation::constants::FRAME_PASS_RATE) 184 |
                << "Less than 90% of valid frames passed similarity check";
            185 |
        }
        else {
            186 | std::cout << "Warning: No faces detected in any sampled frame." << std::endl;
            187 |
        }
        188 |
    }
    189 |
};
190 | 191 | TEST_F(PipelineRunnerVideoTest, ProcessVideoStrictMemoryOneStep) {
    192 | if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        193 | GTEST_SKIP() << "Test assets not found.";
        194 |
    }
    195 | 196 | config::AppConfig app_config;
    197 | auto runner = create_pipeline_runner(app_config);
    198 | 199 | config::TaskConfig task_config;
    200 | task_config.config_version = "1.0";
    201 | task_config.task_info.id = "test_video_strict";
    202 | task_config.io.source_paths.push_back(source_path.string());
    203 | task_config.io.target_paths.push_back(video_path.string());
    204 | 205 | task_config.io.output.path = output_dir.string();
    206 | task_config.io.output.prefix = "pipeline_video_strict_memory_";
    207 | task_config.io.output.suffix = ""; // pipeline_video_strict_memory_slideshow_scaled.mp4
    208 | 209 |                              // Enable Strict Memory
        210 | task_config.resource.memory_strategy = config::MemoryStrategy::Strict;
    211 | 212 | // Adding two steps to verify multi-pass
        213 | config::PipelineStep step1;
    214 | step1.step = "face_swapper";
    215 | step1.enabled = true;
    216 | config::FaceSwapperParams params1;
    217 | params1.model = "inswapper_128_fp16";
    218 | step1.params = params1;
    219 | task_config.pipeline.push_back(step1);
    220 | 221 | std::string expected_output =
        222 | (output_dir / "pipeline_video_strict_memory_slideshow_scaled.mp4").string();
    223 | if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);
    224 | 225 | auto merged_task_config = config::MergeConfigs(task_config, app_config);
    226 | auto result =
        runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});
    227 | 228 | if (result.is_err()) {
        229 | std::cerr << "Strict Runner Error: " << result.error().message << std::endl;
        230 |
    }
    231 | ASSERT_TRUE(result.is_ok());
    232 | EXPECT_TRUE(std::filesystem::exists(expected_output));
    233 | 234 | EXPECT_FALSE(std::filesystem::exists((output_dir / "temp_step_0.mp4").string()));
    235 |
}
236 | 237 | TEST_F(PipelineRunnerVideoTest, ProcessVideoTolerantMemoryOneStep) {
    238 | if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        239 | GTEST_SKIP() << "Test assets not found.";
        240 |
    }
    241 | 242 | config::AppConfig app_config;
    243 | auto runner = create_pipeline_runner(app_config);
    244 | 245 | config::TaskConfig task_config;
    246 | task_config.config_version = "1.0";
    247 | task_config.task_info.id = "test_video_tolerant";
    248 | task_config.io.source_paths.push_back(source_path.string());
    249 | task_config.io.target_paths.push_back(video_path.string());
    250 | 251 | task_config.io.output.path = output_dir.string();
    252 | task_config.io.output.prefix = "pipeline_video_tolerant_memory_";
    253 | task_config.io.output.suffix = ""; // pipeline_video_tolerant_memory_slideshow_scaled.mp4
    254 | 255 |                              // Enable Tolerant Memory
        256 | task_config.resource.memory_strategy = config::MemoryStrategy::Tolerant;
    257 | 258 | // Adding two steps to verify multi-pass
        259 | config::PipelineStep step1;
    260 | step1.step = "face_swapper";
    261 | step1.enabled = true;
    262 | config::FaceSwapperParams params1;
    263 | params1.model = "inswapper_128_fp16";
    264 | step1.params = params1;
    265 | task_config.pipeline.push_back(step1);
    266 | 267 | std::string expected_output =
        268 | (output_dir / "pipeline_video_tolerant_memory_slideshow_scaled.mp4").string();
    269 | if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);
    270 | 271 | auto merged_task_config = config::MergeConfigs(task_config, app_config);
    272 | auto result =
        runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});
    273 | 274 | if (result.is_err()) {
        275 | std::cerr << "Tolerant Runner Error: " << result.error().message << std::endl;
        276 |
    }
    277 | ASSERT_TRUE(result.is_ok());
    278 | EXPECT_TRUE(std::filesystem::exists(expected_output));
    279 | 280 | EXPECT_FALSE(std::filesystem::exists((output_dir / "temp_step_0.mp4").string()));
    281 |
}
282 | 283 | TEST_F(PipelineRunnerVideoTest, ProcessVideoSequentialMultiStep) {
    284 | if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        285 | GTEST_SKIP() << "Test assets not found.";
        286 |
    }
    287 | 288 | config::AppConfig app_config;
    289 | auto runner = create_pipeline_runner(app_config);
    290 | 291 | config::TaskConfig task_config;
    292 | task_config.config_version = "1.0";
    293 | task_config.task_info.id = "test_video_seq_multi_step";
    294 | task_config.io.source_paths.push_back(source_path.string());
    295 | task_config.io.target_paths.push_back(video_path.string());
    296 | 297 | task_config.io.output.path = output_dir.string();
    298 | task_config.io.output.prefix = "pipeline_video_sequential_multi_step_";
    299 | task_config.io.output.suffix = "";
    300 | 301 | task_config.resource.execution_order = config::ExecutionOrder::Sequential;
    302 | 303 | // 1. Swapper
        304 | config::PipelineStep step1;
    305 | step1.step = "face_swapper";
    306 | step1.enabled = true;
    307 | config::FaceSwapperParams params1;
    308 | params1.model = "inswapper_128_fp16";
    309 | step1.params = params1;
    310 | task_config.pipeline.push_back(step1);
    311 | 312 | // 2. Face Enhancer
        313 | config::PipelineStep step2;
    314 | step2.step = "face_enhancer";
    315 | step2.enabled = true;
    316 | config::FaceEnhancerParams params2;
    317 | params2.model = "gfpgan_1.4";
    318 | step2.params = params2;
    319 | task_config.pipeline.push_back(step2);
    320 | 321 | // 3. Expression Restorer
        322 | config::PipelineStep step3;
    323 | step3.step = "expression_restorer";
    324 | step3.enabled = true;
    325 | config::ExpressionRestorerParams params3;
    326 | step3.params = params3;
    327 | task_config.pipeline.push_back(step3);
    328 | 329 | // 4. Frame Enhancer
        330 | config::PipelineStep step4;
    331 | step4.step = "frame_enhancer";
    332 | step4.enabled = true;
    333 | config::FrameEnhancerParams params4;
    334 | params4.model = "real_esrgan_x2_fp16";
    335 | step4.params = params4;
    336 | task_config.pipeline.push_back(step4);
    337 | 338 | std::string expected_output =
        339 | (output_dir / "pipeline_video_sequential_multi_step_slideshow_scaled.mp4").string();
    340 | if (std::filesystem::exists(expected_output)) std::filesystem::remove(expected_output);
    341 | 342 | auto merged_task_config = config::MergeConfigs(task_config, app_config);
    343 | auto result =
        runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});
    344 | 345 | if (result.is_err()) {
        346
            | std::cerr << "Sequential MultiStep Runner Error: " << result.error().message
                        << std::endl;
        347 |
    }
    348 | ASSERT_TRUE(result.is_ok());
    349 | EXPECT_TRUE(std::filesystem::exists(expected_output));
    350 | 351 | // Verify Content (Expected 2x upscale)
        352 | VerifyVideoContent(expected_output, source_path, 2.0f);
    353 |
}
354 | 355 | TEST_F(PipelineRunnerVideoTest, ProcessVideoBatchMutiStep) {
    356 | if (!std::filesystem::exists(video_path) || !std::filesystem::exists(source_path)) {
        357 | GTEST_SKIP() << "Test assets not found.";
        358 |
    }
    359 | 360 | config::AppConfig app_config;
    361 | auto runner = create_pipeline_runner(app_config);
    362 | 363 | config::TaskConfig task_config;
    364 | task_config.config_version = "1.0";
    365 | task_config.task_info.id = "test_video_batch_multi_step";
    366 | task_config.io.source_paths.push_back(source_path.string());
    367 | 368 | // Add two targets to verify batch iteration (even if implementation is currently
                // sequential)
        369 | task_config.io.target_paths.push_back(video_path.string());
    370 | // Create copy for batch test
        371 | std::filesystem::path video_path_2 = output_dir / "slideshow_copy.mp4";
    372
        | std::filesystem::copy_file(video_path, video_path_2,
                                     373 | std::filesystem::copy_options::overwrite_existing);
    374 | task_config.io.target_paths.push_back(video_path_2.string());
    375 | 376 | task_config.io.output.path = output_dir.string();
    377 | task_config.io.output.prefix = "pipeline_video_batch_multi_step_";
    378 | task_config.io.output.suffix = "";
    379 | 380 | task_config.resource.execution_order = config::ExecutionOrder::Batch;
    381 | 382 | // 1. Swapper
        383 | config::PipelineStep step1;
    384 | step1.step = "face_swapper";
    385 | step1.enabled = true;
    386 | config::FaceSwapperParams params1;
    387 | params1.model = "inswapper_128_fp16";
    388 | step1.params = params1;
    389 | task_config.pipeline.push_back(step1);
    390 | 391 | // 2. Face Enhancer
        392 | config::PipelineStep step2;
    393 | step2.step = "face_enhancer";
    394 | step2.enabled = true;
    395 | config::FaceEnhancerParams params2;
    396 | params2.model = "gfpgan_1.4";
    397 | step2.params = params2;
    398 | task_config.pipeline.push_back(step2);
    399 | 400 | // 3. Expression Restorer
        401 | config::PipelineStep step3;
    402 | step3.step = "expression_restorer";
    403 | step3.enabled = true;
    404 | config::ExpressionRestorerParams params3;
    405 | step3.params = params3;
    406 | task_config.pipeline.push_back(step3);
    407 | 408 | // 4. Frame Enhancer
        409 | config::PipelineStep step4;
    410 | step4.step = "frame_enhancer";
    411 | step4.enabled = true;
    412 | config::FrameEnhancerParams params4;
    413 | params4.model = "real_esrgan_x2_fp16";
    414 | step4.params = params4;
    415 | task_config.pipeline.push_back(step4);
    416 | 417 | std::string expected_output_1 =
        418 | (output_dir / "pipeline_video_batch_multi_step_slideshow_scaled.mp4").string();
    419 | std::string expected_output_2 =
        420 | (output_dir / "pipeline_video_batch_multi_step_slideshow_copy.mp4").string();
    421 | 422
        | if (std::filesystem::exists(expected_output_1))
            std::filesystem::remove(expected_output_1);
    423
        | if (std::filesystem::exists(expected_output_2))
            std::filesystem::remove(expected_output_2);
    424 | 425 | auto merged_task_config = config::MergeConfigs(task_config, app_config);
    426 | auto result =
        runner->run(merged_task_config, [](const services::pipeline::TaskProgress& p) {});
    427 | 428 | if (result.is_err()) {
        429 | std::cerr << "Batch MultiStep Runner Error: " << result.error().message << std::endl;
        430 |
    }
    431 | ASSERT_TRUE(result.is_ok());
    432 | EXPECT_TRUE(std::filesystem::exists(expected_output_1));
    433 | EXPECT_TRUE(std::filesystem::exists(expected_output_2));
    434 | 435 | // Verify Content (Expected 2x upscale)
        436 | VerifyVideoContent(expected_output_1, source_path, 2.0f);
    437 | // Optionally verify second output too, but one is usually enough for pipeline logic check
        438 |
}
439 | 440 | // ============================================================================
    441 |   // Performance Tests (Merged from E2E)
    442 |   // ============================================================================
    443 | 444 | TEST_F(PipelineRunnerVideoTest, ProcessVideoAchievesMinimumFPS) {
    445 | auto input_info = get_video_info(video_path);
    446 | auto output_path = output_dir / "result_slideshow_fps.mp4";
    447 | 448 | config::TaskConfig task_config;
    449 | task_config.task_info.id = "video_720p_fps_test";
    450 | task_config.io.source_paths = {source_path.string()};
    451 | task_config.io.target_paths = {video_path.string()};
    452 | task_config.io.output.path = output_dir.string();
    453 | task_config.io.output.prefix = "result_";
    454 | 455 | config::PipelineStep swap_step;
    456 | swap_step.step = "face_swapper";
    457 | swap_step.enabled = true;
    458 | config::FaceSwapperParams params;
    459 | params.model = "inswapper_128_fp16";
    460 | swap_step.params = params;
    461 | task_config.pipeline.push_back(swap_step);
    462 | 463 | config::AppConfig app_config;
    464 | 465 | auto start = steady_clock::now();
    466 | auto runner = create_pipeline_runner(app_config);
    467 | auto merged_config = config::MergeConfigs(task_config, app_config);
    468 | auto result =
        runner->run(merged_config, [](const services::pipeline::TaskProgress& /*p*/) {});
    469 | auto duration_ms = duration_cast<milliseconds>(steady_clock::now() - start).count();
    470 | 471 | ASSERT_TRUE(result.is_ok());
    472 | 473 | // Calculate actual FPS
        474 | double actual_fps = (input_info.frame_count * 1000.0) / duration_ms;
    475 | 476 | std::cout << "=== Performance Summary ===" << std::endl;
    477 | std::cout << "Total frames: " << input_info.frame_count << std::endl;
    478 | std::cout << "Duration: " << duration_ms << " ms" << std::endl;
    479 | std::cout << "Actual FPS: " << actual_fps << std::endl;
    480 | 481 | #ifdef NDEBUG 482
        | EXPECT_GE(actual_fps, tests::helpers::foundation::constants::MIN_FPS_RTX4060) 483 |
        << "FPS below threshold: " << actual_fps 484 |
        << " (min: " << tests::helpers::foundation::constants::MIN_FPS_RTX4060 << ")";
    485 | #else 486
        | std::cout << "[WARN] Running in DEBUG mode. FPS requirement ignored. Got: "
                    << actual_fps 487
        | << std::endl;
    488 | #endif 489 |
}
490 | 491 | TEST_F(PipelineRunnerVideoTest, ProcessVideoCompletesWithinTimeLimit) {
    492 | config::TaskConfig task_config;
    493 | task_config.task_info.id = "video_720p_time_test";
    494 | task_config.io.source_paths = {source_path.string()};
    495 | task_config.io.target_paths = {video_path.string()};
    496 | task_config.io.output.path = output_dir.string();
    497 | task_config.io.output.prefix = "result_";
    498 | 499 | config::PipelineStep swap_step;
    500 | swap_step.step = "face_swapper";
    501 |