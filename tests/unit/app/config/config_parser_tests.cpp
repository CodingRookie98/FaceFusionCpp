#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

import config.parser;

using namespace config;
using namespace testing;

// ============================================================================
// Result<T, E> 类型测试
// ============================================================================

TEST(ResultTest, OkValue) {
    auto result = Result<int>::Ok(42);
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_err());
    EXPECT_EQ(result.value(), 42);
    EXPECT_TRUE(static_cast<bool>(result));
}

TEST(ResultTest, ErrValue) {
    auto result = Result<int>::Err(ConfigError("test error", "field"));
    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error().message, "test error");
    EXPECT_EQ(result.error().field, "field");
    EXPECT_FALSE(static_cast<bool>(result));
}

TEST(ResultTest, ValueOr) {
    auto ok_result = Result<int>::Ok(42);
    auto err_result = Result<int>::Err(ConfigError("error"));

    EXPECT_EQ(ok_result.value_or(0), 42);
    EXPECT_EQ(err_result.value_or(0), 0);
}

TEST(ResultTest, VoidSpecialization) {
    auto ok_result = Result<void>::Ok();
    auto err_result = Result<void>::Err(ConfigError("error"));

    EXPECT_TRUE(ok_result.is_ok());
    EXPECT_TRUE(err_result.is_err());
    EXPECT_EQ(err_result.error().message, "error");
}

// ============================================================================
// 枚举解析测试
// ============================================================================

TEST(EnumParsingTest, MemoryStrategy) {
    EXPECT_TRUE(ParseMemoryStrategy("strict").is_ok());
    EXPECT_EQ(ParseMemoryStrategy("strict").value(), MemoryStrategy::Strict);
    EXPECT_EQ(ParseMemoryStrategy("Strict").value(), MemoryStrategy::Strict);
    EXPECT_EQ(ParseMemoryStrategy("TOLERANT").value(), MemoryStrategy::Tolerant);
    EXPECT_TRUE(ParseMemoryStrategy("invalid").is_err());
}

TEST(EnumParsingTest, DownloadStrategy) {
    EXPECT_EQ(ParseDownloadStrategy("force").value(), DownloadStrategy::Force);
    EXPECT_EQ(ParseDownloadStrategy("skip").value(), DownloadStrategy::Skip);
    EXPECT_EQ(ParseDownloadStrategy("auto").value(), DownloadStrategy::Auto);
    EXPECT_TRUE(ParseDownloadStrategy("invalid").is_err());
}

TEST(EnumParsingTest, ExecutionOrder) {
    EXPECT_EQ(ParseExecutionOrder("sequential").value(), ExecutionOrder::Sequential);
    EXPECT_EQ(ParseExecutionOrder("batch").value(), ExecutionOrder::Batch);
    EXPECT_TRUE(ParseExecutionOrder("invalid").is_err());
}

TEST(EnumParsingTest, ConflictPolicy) {
    EXPECT_EQ(ParseConflictPolicy("overwrite").value(), ConflictPolicy::Overwrite);
    EXPECT_EQ(ParseConflictPolicy("rename").value(), ConflictPolicy::Rename);
    EXPECT_EQ(ParseConflictPolicy("error").value(), ConflictPolicy::Error);
    EXPECT_TRUE(ParseConflictPolicy("invalid").is_err());
}

TEST(EnumParsingTest, FaceSelectorMode) {
    EXPECT_EQ(ParseFaceSelectorMode("reference").value(), FaceSelectorMode::Reference);
    EXPECT_EQ(ParseFaceSelectorMode("one").value(), FaceSelectorMode::One);
    EXPECT_EQ(ParseFaceSelectorMode("many").value(), FaceSelectorMode::Many);
    EXPECT_TRUE(ParseFaceSelectorMode("invalid").is_err());
}

TEST(EnumParsingTest, LogLevel) {
    EXPECT_EQ(ParseLogLevel("trace").value(), LogLevel::Trace);
    EXPECT_EQ(ParseLogLevel("debug").value(), LogLevel::Debug);
    EXPECT_EQ(ParseLogLevel("info").value(), LogLevel::Info);
    EXPECT_EQ(ParseLogLevel("warn").value(), LogLevel::Warn);
    EXPECT_EQ(ParseLogLevel("error").value(), LogLevel::Error);
    EXPECT_TRUE(ParseLogLevel("invalid").is_err());
}

// ============================================================================
// 枚举 ToString 测试
// ============================================================================

TEST(EnumToStringTest, AllEnums) {
    EXPECT_EQ(ToString(MemoryStrategy::Strict), "strict");
    EXPECT_EQ(ToString(MemoryStrategy::Tolerant), "tolerant");
    EXPECT_EQ(ToString(DownloadStrategy::Auto), "auto");
    EXPECT_EQ(ToString(ExecutionOrder::Sequential), "sequential");
    EXPECT_EQ(ToString(ConflictPolicy::Error), "error");
    EXPECT_EQ(ToString(FaceSelectorMode::Many), "many");
    EXPECT_EQ(ToString(LogLevel::Info), "info");
    EXPECT_EQ(ToString(LogRotation::Daily), "daily");
}

// ============================================================================
// AppConfig 解析测试
// ============================================================================

TEST(AppConfigParsingTest, ValidYaml) {
    const std::string yaml = R"(
config_version: "1.0"
inference:
  device_id: 0
  engine_cache:
    enable: true
    path: "./.cache/tensorrt"
  default_providers:
    - tensorrt
    - cuda
    - cpu
resource:
  memory_strategy: strict
logging:
  level: info
  directory: "./logs"
  rotation: daily
models:
  path: "./assets/models"
  download_strategy: auto
temp_directory: "./temp"
)";

    auto result = ParseAppConfigFromString(yaml);
    ASSERT_TRUE(result.is_ok()) << result.error().message;

    const auto& config = result.value();
    EXPECT_EQ(config.config_version, "1.0");
    EXPECT_EQ(config.inference.device_id, 0);
    EXPECT_TRUE(config.inference.engine_cache.enable);
    EXPECT_EQ(config.inference.engine_cache.path, "./.cache/tensorrt");
    EXPECT_EQ(config.inference.default_providers.size(), 3);
    EXPECT_EQ(config.resource.memory_strategy, MemoryStrategy::Strict);
    EXPECT_EQ(config.logging.level, LogLevel::Info);
    EXPECT_EQ(config.logging.directory, "./logs");
    EXPECT_EQ(config.models.path, "./assets/models");
    EXPECT_EQ(config.models.download_strategy, DownloadStrategy::Auto);
    EXPECT_EQ(config.temp_directory, "./temp");
}

TEST(AppConfigParsingTest, DefaultValues) {
    const std::string yaml = R"(
config_version: "1.0"
)";

    auto result = ParseAppConfigFromString(yaml);
    ASSERT_TRUE(result.is_ok());

    const auto& config = result.value();
    // Check defaults are applied
    EXPECT_EQ(config.resource.memory_strategy, MemoryStrategy::Strict);
    EXPECT_EQ(config.logging.level, LogLevel::Info);
    EXPECT_EQ(config.models.download_strategy, DownloadStrategy::Auto);
}

TEST(AppConfigParsingTest, InvalidMemoryStrategy) {
    const std::string yaml = R"(
config_version: "1.0"
resource:
  memory_strategy: invalid_value
)";

    auto result = ParseAppConfigFromString(yaml);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().message, HasSubstr("Invalid memory_strategy"));
}

// ============================================================================
// AppConfig 校验测试
// ============================================================================

TEST(AppConfigValidationTest, ValidConfig) {
    AppConfig config;
    config.config_version = "1.0";
    config.models.path = "./models";
    config.logging.directory = "./logs";

    auto result = ValidateAppConfig(config);
    EXPECT_TRUE(result.is_ok());
}

TEST(AppConfigValidationTest, InvalidVersion) {
    AppConfig config;
    config.config_version = "2.0";
    config.models.path = "./models";
    config.logging.directory = "./logs";

    auto result = ValidateAppConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().message, HasSubstr("Unsupported config version"));
}

TEST(AppConfigValidationTest, EmptyModelsPath) {
    AppConfig config;
    config.config_version = "1.0";
    config.models.path = "";
    config.logging.directory = "./logs";

    auto result = ValidateAppConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().field, Eq("models.path"));
}

// ============================================================================
// TaskConfig 解析测试
// ============================================================================

TEST(TaskConfigParsingTest, ValidYaml) {
    const std::string yaml = R"(
config_version: "1.0"
task_info:
  id: test_task_001
  description: "Test task"
  enable_logging: true
  enable_resume: false
io:
  source_paths:
    - "/path/to/source.jpg"
  target_paths:
    - "/path/to/target.mp4"
  output:
    path: "/path/to/output"
    prefix: "result_"
    image_format: png
    video_encoder: libx264
    video_quality: 80
    conflict_policy: error
    audio_policy: copy
resource:
  thread_count: 4
  execution_order: sequential
  segment_duration_seconds: 0
face_analysis:
  face_detector:
    models:
      - yoloface
    score_threshold: 0.5
  face_landmarker:
    model: 2dfan4
  face_recognizer:
    model: arcface_w600k_r50
    similarity_threshold: 0.6
  face_masker:
    types:
      - box
      - occlusion
    region:
      - face
pipeline:
  - step: face_swapper
    name: swap_step
    enabled: true
    params:
      model: inswapper_128
      face_selector_mode: many
  - step: face_enhancer
    name: enhance_step
    enabled: true
    params:
      model: gfpgan_1.4
      blend_factor: 0.8
)";

    auto result = ParseTaskConfigFromString(yaml);
    ASSERT_TRUE(result.is_ok()) << result.error().message;

    const auto& config = result.value();
    EXPECT_EQ(config.config_version, "1.0");
    EXPECT_EQ(config.task_info.id, "test_task_001");
    EXPECT_EQ(config.task_info.description, "Test task");
    EXPECT_TRUE(config.task_info.enable_logging);
    EXPECT_FALSE(config.task_info.enable_resume);

    EXPECT_EQ(config.io.source_paths.size(), 1);
    EXPECT_EQ(config.io.target_paths.size(), 1);
    EXPECT_EQ(config.io.output.path, "/path/to/output");
    EXPECT_EQ(config.io.output.conflict_policy, ConflictPolicy::Error);

    EXPECT_EQ(config.resource.thread_count, 4);
    EXPECT_EQ(config.resource.execution_order, ExecutionOrder::Sequential);

    EXPECT_EQ(config.face_analysis.face_detector.score_threshold, 0.5);
    EXPECT_EQ(config.face_analysis.face_recognizer.similarity_threshold, 0.6);

    ASSERT_EQ(config.pipeline.size(), 2);
    EXPECT_EQ(config.pipeline[0].step, "face_swapper");
    EXPECT_EQ(config.pipeline[0].name, "swap_step");
    EXPECT_EQ(config.pipeline[1].step, "face_enhancer");
}

TEST(TaskConfigParsingTest, InvalidStepType) {
    const std::string yaml = R"(
config_version: "1.0"
task_info:
  id: test
io:
  output:
    path: "/output"
pipeline:
  - step: unknown_processor
    name: test
    params:
      model: test
)";

    auto result = ParseTaskConfigFromString(yaml);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().message, HasSubstr("Unknown pipeline step type"));
}

// ============================================================================
// TaskConfig 校验测试
// ============================================================================

TEST(TaskConfigValidationTest, ValidConfig) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test_task";
    config.io.output.path = "/output";
    config.io.output.video_quality = 80;
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_ok()) << result.error().message;
}

TEST(TaskConfigValidationTest, EmptyPipeline) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.output.path = "/output";
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;
    // Empty pipeline

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().message, HasSubstr("at least one step"));
}

TEST(TaskConfigValidationTest, InvalidVideoQuality) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.output.path = "/output";
    config.io.output.video_quality = 150; // Invalid
    config.face_analysis.face_detector.score_threshold = 0.5;
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_err());
    EXPECT_THAT(result.error().field, Eq("io.output.video_quality"));
}

TEST(TaskConfigValidationTest, InvalidScoreThreshold) {
    TaskConfig config;
    config.config_version = "1.0";
    config.task_info.id = "test";
    config.io.output.path = "/output";
    config.face_analysis.face_detector.score_threshold = 1.5; // Invalid
    config.face_analysis.face_recognizer.similarity_threshold = 0.6;

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{};
    config.pipeline.push_back(step);

    auto result = ValidateTaskConfig(config);
    EXPECT_TRUE(result.is_err());
}
