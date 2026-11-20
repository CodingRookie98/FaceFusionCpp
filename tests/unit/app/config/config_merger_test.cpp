#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

import config.merger;
import config.types;

using namespace config;
using namespace testing;

TEST(ConfigMergerTest, MergeIODefaults) {
    TaskConfig task;
    task.config_version = "1.0";
    // video_quality is 0 (sentinel) in task

    AppConfig app;
    app.default_task_settings.io.output.video_quality = 95;
    app.default_task_settings.io.output.video_encoder = "h265";

    auto result = MergeConfigs(task, app);

    EXPECT_EQ(result.io.output.video_quality, 95);
    EXPECT_EQ(result.io.output.video_encoder, "h265");
    EXPECT_EQ(result.io.output.prefix, "result_"); // Hardcoded fallback
}

TEST(ConfigMergerTest, TaskPriorityOverApp) {
    TaskConfig task;
    task.config_version = "1.0";
    task.io.output.video_quality = 70;

    AppConfig app;
    app.default_task_settings.io.output.video_quality = 95;

    auto result = MergeConfigs(task, app);

    EXPECT_EQ(result.io.output.video_quality, 70);
}

TEST(ConfigMergerTest, ApplyDefaultModels) {
    TaskConfig task;
    task.config_version = "1.0";

    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{}; // model is empty
    task.pipeline.push_back(step);

    AppConfig app;
    app.default_models.face_swapper = "custom_swapper";

    auto result = MergeConfigs(task, app);

    const auto& merged_step = result.pipeline[0];
    const auto* params = std::get_if<FaceSwapperParams>(&merged_step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->model, "custom_swapper");
}

TEST(ConfigMergerTest, ModelTaskPriority) {
    TaskConfig task;
    task.config_version = "1.0";

    PipelineStep step;
    step.step = "face_swapper";
    FaceSwapperParams params;
    params.model = "task_swapper";
    step.params = params;
    task.pipeline.push_back(step);

    AppConfig app;
    app.default_models.face_swapper = "app_swapper";

    auto result = MergeConfigs(task, app);

    const auto& merged_step = result.pipeline[0];
    const auto* merged_params = std::get_if<FaceSwapperParams>(&merged_step.params);
    ASSERT_NE(merged_params, nullptr);
    EXPECT_EQ(merged_params->model, "task_swapper");
}

// ─────────────────────────────────────────────────────────────────────────
// ApplyCliParamsToStep: 快捷模式 CLI 处理器参数 → step.params 转换
// ─────────────────────────────────────────────────────────────────────────

TEST(ConfigMergerTest, ApplyCliParamsToStepSwapper) {
    PipelineStep step;
    step.step = "face_swapper";
    step.cli_params = {{"model", "inswapper_128"},
                       {"face_selector_mode", "reference"},
                       {"reference_face_path", "ref.jpg"}};

    auto result = ApplyCliParamsToStep(step);
    ASSERT_TRUE(result.is_ok());

    const auto* params = std::get_if<FaceSwapperParams>(&step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->model, "inswapper_128");
    EXPECT_EQ(params->face_selector_mode, FaceSelectorMode::Reference);
    ASSERT_TRUE(params->reference_face_path.has_value());
    EXPECT_EQ(*params->reference_face_path, "ref.jpg");
}

TEST(ConfigMergerTest, ApplyCliParamsToStepEnhancerNumeric) {
    PipelineStep step;
    step.step = "face_enhancer";
    step.cli_params = {{"model", "gfpgan_1.4"}, {"blend_factor", "0.9"}};

    auto result = ApplyCliParamsToStep(step);
    ASSERT_TRUE(result.is_ok());

    const auto* params = std::get_if<FaceEnhancerParams>(&step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->model, "gfpgan_1.4");
    EXPECT_DOUBLE_EQ(params->blend_factor, 0.9);
}

TEST(ConfigMergerTest, ApplyCliParamsToStepExpressionRestorer) {
    PipelineStep step;
    step.step = "expression_restorer";
    step.cli_params = {{"model", "live_portrait"}, {"restore_factor", "0.5"}};

    auto result = ApplyCliParamsToStep(step);
    ASSERT_TRUE(result.is_ok());

    const auto* params = std::get_if<ExpressionRestorerParams>(&step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->model, "live_portrait");
    EXPECT_DOUBLE_EQ(params->restore_factor, 0.5);
}

TEST(ConfigMergerTest, ApplyCliParamsToStepFrameEnhancer) {
    PipelineStep step;
    step.step = "frame_enhancer";
    step.cli_params = {{"model", "real_esrgan_x8"}, {"enhance_factor", "0.7"}};

    auto result = ApplyCliParamsToStep(step);
    ASSERT_TRUE(result.is_ok());

    const auto* params = std::get_if<FrameEnhancerParams>(&step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->model, "real_esrgan_x8");
    EXPECT_DOUBLE_EQ(params->enhance_factor, 0.7);
}

TEST(ConfigMergerTest, ApplyCliParamsToStepInvalidEnum) {
    PipelineStep step;
    step.step = "face_swapper";
    step.cli_params = {{"face_selector_mode", "invalid_mode"}};

    auto result = ApplyCliParamsToStep(step);
    EXPECT_TRUE(result.is_err());
}

TEST(ConfigMergerTest, ApplyCliParamsToStepEmptyNoOp) {
    PipelineStep step;
    step.step = "face_swapper";
    step.params = FaceSwapperParams{}; // default: model empty

    auto result = ApplyCliParamsToStep(step);
    ASSERT_TRUE(result.is_ok());

    const auto* params = std::get_if<FaceSwapperParams>(&step.params);
    ASSERT_NE(params, nullptr);
    EXPECT_TRUE(params->model.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
