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

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
