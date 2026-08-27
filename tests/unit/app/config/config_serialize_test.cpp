#include <gtest/gtest.h>
#include <string>
#include <nlohmann/json.hpp>

import config.parser;
import config.types;

using namespace config;
using namespace testing;

// ============================================================================
// TaskConfig JSON 序列化/反序列化测试
// ============================================================================

namespace {

TaskConfig MakeFullConfig() {
    TaskConfig cfg;
    cfg.config_version = "0.34.1";

    cfg.task_info.id = "task_serialize_test";
    cfg.task_info.description = "roundtrip test";
    cfg.task_info.enable_logging = true;
    cfg.task_info.enable_resume = false;

    cfg.io.source_paths = {"src1.jpg", "src2.jpg"};
    cfg.io.target_paths = {"tgt1.mp4"};
    cfg.io.output.path = "./output/task_x";
    cfg.io.output.prefix = "result_";
    cfg.io.output.suffix = "";
    cfg.io.output.image_format = "png";
    cfg.io.output.video_encoder = "libx264";
    cfg.io.output.video_quality = 80;
    cfg.io.output.conflict_policy = ConflictPolicy::Overwrite;
    cfg.io.output.audio_policy = AudioPolicy::Copy;

    cfg.resource.thread_count = 4;
    cfg.resource.max_queue_size = 10;
    cfg.resource.execution_order = ExecutionOrder::Sequential;
    cfg.resource.memory_strategy = MemoryStrategy::Strict;
    cfg.resource.segment_duration_seconds = 0;
    cfg.resource.max_frames = 0;

    // 4 种 processor 全覆盖
    config::PipelineStep swapper;
    swapper.step = "face_swapper";
    swapper.name = "主角";
    swapper.enabled = true;
    swapper.params = config::FaceSwapperParams{
        .model = "inswapper_128_fp16",
        .face_selector_mode = FaceSelectorMode::Reference,
        .reference_face_path = "assets/lenna.bmp",
    };
    cfg.pipeline.push_back(std::move(swapper));

    config::PipelineStep enhancer;
    enhancer.step = "face_enhancer";
    enhancer.name = "增强";
    enhancer.enabled = false;
    enhancer.params = config::FaceEnhancerParams{
        .model = "gfpgan_1.4",
        .blend_factor = 0.85,
        .face_selector_mode = FaceSelectorMode::Many,
        .reference_face_path = std::nullopt,
    };
    cfg.pipeline.push_back(std::move(enhancer));

    config::PipelineStep restorer;
    restorer.step = "expression_restorer";
    restorer.name = "微表情";
    restorer.enabled = true;
    restorer.params = config::ExpressionRestorerParams{
        .model = "live_portrait",
        .restore_factor = 0.6,
        .face_selector_mode = FaceSelectorMode::One,
        .reference_face_path = "assets/avatar.png",
    };
    cfg.pipeline.push_back(std::move(restorer));

    config::PipelineStep frame;
    frame.step = "frame_enhancer";
    frame.name = "超分";
    frame.enabled = true;
    frame.params =
        config::FrameEnhancerParams{.model = "real_esrgan_x2_fp16", .enhance_factor = 0.9};
    cfg.pipeline.push_back(std::move(frame));

    return cfg;
}

} // namespace

// 1. 往返一致性：全部字段逐一相等
TEST(TaskConfigSerializeTest, RoundTripPreservesAllFields) {
    auto cfg = MakeFullConfig();

    auto json = SerializeTaskConfig(cfg);
    ASSERT_TRUE(json.is_ok()) << json.error().message;

    auto restored = DeserializeTaskConfig(json.value());
    ASSERT_TRUE(restored.is_ok()) << restored.error().message;

    const auto& out = restored.value();
    EXPECT_EQ(out.config_version, cfg.config_version);
    EXPECT_EQ(out.task_info.id, cfg.task_info.id);
    EXPECT_EQ(out.task_info.description, cfg.task_info.description);
    EXPECT_EQ(out.task_info.enable_logging, cfg.task_info.enable_logging);
    EXPECT_EQ(out.task_info.enable_resume, cfg.task_info.enable_resume);

    EXPECT_EQ(out.io.source_paths, cfg.io.source_paths);
    EXPECT_EQ(out.io.target_paths, cfg.io.target_paths);
    EXPECT_EQ(out.io.output.path, cfg.io.output.path);
    EXPECT_EQ(out.io.output.prefix, cfg.io.output.prefix);
    EXPECT_EQ(out.io.output.image_format, cfg.io.output.image_format);
    EXPECT_EQ(out.io.output.video_encoder, cfg.io.output.video_encoder);
    EXPECT_EQ(out.io.output.video_quality, cfg.io.output.video_quality);
    EXPECT_EQ(out.io.output.conflict_policy, cfg.io.output.conflict_policy);
    EXPECT_EQ(out.io.output.audio_policy, cfg.io.output.audio_policy);

    EXPECT_EQ(out.resource.thread_count, cfg.resource.thread_count);
    EXPECT_EQ(out.resource.max_queue_size, cfg.resource.max_queue_size);
    EXPECT_EQ(out.resource.execution_order, cfg.resource.execution_order);
    EXPECT_EQ(out.resource.memory_strategy, cfg.resource.memory_strategy);

    ASSERT_EQ(out.pipeline.size(), cfg.pipeline.size());
    for (std::size_t i = 0; i < cfg.pipeline.size(); ++i) {
        const auto& a = out.pipeline[i];
        const auto& b = cfg.pipeline[i];
        EXPECT_EQ(a.step, b.step);
        EXPECT_EQ(a.name, b.name);
        EXPECT_EQ(a.enabled, b.enabled);
        EXPECT_EQ(a.params.index(), b.params.index());
    }

    // variant 内容逐一校验
    const auto& swapper = std::get<FaceSwapperParams>(out.pipeline[0].params);
    EXPECT_EQ(swapper.model, "inswapper_128_fp16");
    EXPECT_EQ(swapper.face_selector_mode, FaceSelectorMode::Reference);
    ASSERT_TRUE(swapper.reference_face_path.has_value());
    EXPECT_EQ(swapper.reference_face_path.value(), "assets/lenna.bmp");

    const auto& enhancer = std::get<FaceEnhancerParams>(out.pipeline[1].params);
    EXPECT_EQ(enhancer.model, "gfpgan_1.4");
    EXPECT_DOUBLE_EQ(enhancer.blend_factor, 0.85);
    EXPECT_EQ(enhancer.face_selector_mode, FaceSelectorMode::Many);
    EXPECT_FALSE(enhancer.reference_face_path.has_value());

    const auto& restorer = std::get<ExpressionRestorerParams>(out.pipeline[2].params);
    EXPECT_EQ(restorer.model, "live_portrait");
    EXPECT_DOUBLE_EQ(restorer.restore_factor, 0.6);
    EXPECT_EQ(restorer.face_selector_mode, FaceSelectorMode::One);

    const auto& frame = std::get<FrameEnhancerParams>(out.pipeline[3].params);
    EXPECT_EQ(frame.model, "real_esrgan_x2_fp16");
    EXPECT_DOUBLE_EQ(frame.enhance_factor, 0.9);
}

// 2. variant 分发：JSON 结构含对应 processor 字段
TEST(TaskConfigSerializeTest, VariantDispatchToTypedFields) {
    auto cfg = MakeFullConfig();
    auto json = SerializeTaskConfig(cfg);
    ASSERT_TRUE(json.is_ok());

    const auto& j = json.value();
    ASSERT_TRUE(j.contains("pipeline") && j["pipeline"].is_array());
    ASSERT_EQ(j["pipeline"].size(), 4);

    // face_swapper → model/face_selector_mode/reference_face_path
    const auto& swapper = j["pipeline"][0];
    EXPECT_EQ(swapper["step"], "face_swapper");
    EXPECT_EQ(swapper["params"]["model"], "inswapper_128_fp16");
    EXPECT_EQ(swapper["params"]["face_selector_mode"], "reference");
    EXPECT_EQ(swapper["params"]["reference_face_path"], "assets/lenna.bmp");

    // face_enhancer → blend_factor
    EXPECT_EQ(j["pipeline"][1]["params"]["blend_factor"], 0.85);
    EXPECT_EQ(j["pipeline"][1]["params"]["face_selector_mode"], "many");

    // expression_restorer → restore_factor
    EXPECT_EQ(j["pipeline"][2]["params"]["restore_factor"], 0.6);
    EXPECT_EQ(j["pipeline"][2]["params"]["face_selector_mode"], "one");

    // frame_enhancer → enhance_factor
    EXPECT_EQ(j["pipeline"][3]["params"]["enhance_factor"], 0.9);
}

// 3. 枚举字符串化
TEST(TaskConfigSerializeTest, EnumsSerializeToStrings) {
    auto cfg = MakeFullConfig();
    auto json = SerializeTaskConfig(cfg);
    ASSERT_TRUE(json.is_ok());

    const auto& j = json.value();
    EXPECT_EQ(j["io"]["output"]["conflict_policy"], "overwrite");
    EXPECT_EQ(j["io"]["output"]["audio_policy"], "copy");
    EXPECT_EQ(j["resource"]["execution_order"], "sequential");
    EXPECT_EQ(j["resource"]["memory_strategy"], "strict");
}

// 4. 非法 JSON 反序列化返回错误（不抛异常）
TEST(TaskConfigSerializeTest, DeserializeInvalidJsonReturnsError) {
    auto result = DeserializeTaskConfig(nlohmann::json::parse(R"({"pipeline": "not_an_array"})"));
    EXPECT_TRUE(result.is_err());

    auto missing = DeserializeTaskConfig(nlohmann::json::object());
    EXPECT_TRUE(missing.is_err());
}

// 5. 空 pipeline 可序列化
TEST(TaskConfigSerializeTest, EmptyPipelineRoundTrips) {
    TaskConfig cfg;
    cfg.config_version = "0.34.1";
    cfg.io.source_paths = {"s.jpg"};
    cfg.io.target_paths = {"t.jpg"};

    auto json = SerializeTaskConfig(cfg);
    ASSERT_TRUE(json.is_ok());
    auto restored = DeserializeTaskConfig(json.value());
    ASSERT_TRUE(restored.is_ok());
    EXPECT_TRUE(restored.value().pipeline.empty());
}