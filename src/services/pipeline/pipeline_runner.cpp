/**
 * @file pipeline_runner.cpp
 * @brief PipelineRunner 实现 (简化版)
 */
module;

#include <memory>
#include <atomic>
#include <string>

module services.pipeline.runner;

namespace services::pipeline {

// ============================================================================
// PipelineRunnerImpl - 简化版本
// ============================================================================

class PipelineRunnerImpl : public IPipelineRunner {
public:
    explicit PipelineRunnerImpl(const config::AppConfig& app_config) :
        m_app_config(app_config), m_running(false), m_cancelled(false) {}

    config::Result<void, config::ConfigError> Run(const config::TaskConfig& task_config,
                                                  ProgressCallback progress_callback) override {
        if (m_running.exchange(true)) {
            return config::Result<void, config::ConfigError>::Err(
                config::ConfigError("Pipeline is already running"));
        }
        m_cancelled = false;

        // 验证配置
        auto validate_result = config::ValidateTaskConfig(task_config);
        if (!validate_result) {
            m_running = false;
            return config::Result<void, config::ConfigError>::Err(validate_result.error());
        }

        // TODO: 实际执行逻辑将在后续添加
        // 目前只是验证配置后返回成功

        if (progress_callback) {
            TaskProgress progress;
            progress.task_id = task_config.task_info.id;
            progress.current_frame = 0;
            progress.total_frames = 0;
            progress.current_step = "ready";
            progress.fps = 0.0;
            progress_callback(progress);
        }

        m_running = false;
        return config::Result<void, config::ConfigError>::Ok();
    }

    void Cancel() override { m_cancelled = true; }

    bool IsRunning() const override { return m_running; }

private:
    config::AppConfig m_app_config;
    std::atomic<bool> m_running;
    std::atomic<bool> m_cancelled;
};

// ============================================================================
// Factory
// ============================================================================

std::unique_ptr<IPipelineRunner> CreatePipelineRunner(const config::AppConfig& app_config) {
    return std::make_unique<PipelineRunnerImpl>(app_config);
}

} // namespace services::pipeline
