/**
 * @file pipeline_executor.ixx
 * @brief Production task executor wired to services::pipeline::PipelineRunner
 */
module;

#include <memory>
#include <mutex>

export module app.web.pipeline_executor;

import app.web.task_manager;
import config.app;
import config.task;
import services.pipeline.runner;

export namespace app::web {

/// Executor backed by the real PipelineRunner (supports cancellation)
class PipelineTaskExecutor final : public ITaskExecutor {
public:
    explicit PipelineTaskExecutor(const config::AppConfig& app_config);
    ~PipelineTaskExecutor() override;

    int run(const config::TaskConfig& config,
            const services::pipeline::ProgressCallback& progress) override;
    int run(const config::TaskConfig& config, const services::pipeline::ProgressCallback& progress,
            std::string& error_message) override;
    void cancel() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace app::web
