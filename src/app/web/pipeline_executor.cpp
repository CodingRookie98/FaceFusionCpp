module;
#include <memory>
#include <mutex>
#include <utility>

module app.web.pipeline_executor;

import config.merger;

namespace app::web {

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
        return static_cast<int>(result.error().code);
    }

    return 0;
}

void PipelineTaskExecutor::cancel() {
    std::lock_guard lock(m_impl->mutex);
    if (m_impl->runner) { m_impl->runner->cancel(); }
}

} // namespace app::web
