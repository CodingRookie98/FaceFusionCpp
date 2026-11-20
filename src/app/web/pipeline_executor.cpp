module;
#include <format>
#include <memory>
#include <mutex>
#include <utility>

module app.web.pipeline_executor;

import config.merger;
import foundation.infrastructure.logger;

namespace app::web {

using Logger = foundation::infrastructure::logger::Logger;

struct PipelineTaskExecutor::Impl {
    explicit Impl(const config::AppConfig& cfg) : app_config(cfg) {}

    const config::AppConfig app_config;
    std::mutex mutex;
    std::shared_ptr<services::pipeline::PipelineRunner> runner;
};

PipelineTaskExecutor::PipelineTaskExecutor(const config::AppConfig& app_config) :
    m_impl(std::make_unique<Impl>(app_config)) {}

PipelineTaskExecutor::~PipelineTaskExecutor() = default;

int PipelineTaskExecutor::run(const config::TaskConfig& config,
                              const services::pipeline::ProgressCallback& progress) {
    std::string err;
    return run(config, progress, err);
}

int PipelineTaskExecutor::run(const config::TaskConfig& config,
                              const services::pipeline::ProgressCallback& progress,
                              std::string& error_message) {
    // Merge app defaults (models, io, resource) into the submitted config
    auto merged = config::MergeConfigs(config, m_impl->app_config);

    Logger::get_instance()->info(std::format(
        "[PipelineExecutor] Starting pipeline execution: targets={}, sources={}, processors={}",
        merged.io.target_paths.size(), merged.io.source_paths.size(), merged.pipeline.size()));

    auto runner = services::pipeline::create_pipeline_runner(m_impl->app_config);
    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->runner = std::move(runner); // transfer ownership for cancellation
    }

    auto result = m_impl->runner->run(merged, progress);

    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->runner.reset();
    }

    if (!result) {
        error_message = result.error().message;
        Logger::get_instance()->error(
            std::format("[PipelineExecutor] Pipeline execution failed (code={}): {}",
                        static_cast<int>(result.error().code), error_message));
        return static_cast<int>(result.error().code);
    }

    Logger::get_instance()->info("[PipelineExecutor] Pipeline execution completed successfully");
    return 0;
}

void PipelineTaskExecutor::cancel() {
    std::lock_guard lock(m_impl->mutex);
    if (m_impl->runner) {
        Logger::get_instance()->warn("[PipelineExecutor] Cancelling active pipeline runner");
        m_impl->runner->cancel();
    }
}

} // namespace app::web
