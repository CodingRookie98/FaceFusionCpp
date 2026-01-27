/**
 * @file pipeline_runner.ixx
 * @brief High-level runner for configuration-driven processing
 * @author CodingRookie
 * @date 2026-01-27
 */
module;

#include <functional>
#include <memory>
#include <string>
#include <cstddef>

export module services.pipeline.runner;

export import config.types;
export import config.app;
export import config.task;
import config.parser;
export import :types;

export namespace services::pipeline {

/**
 * @brief Orchestrates the end-to-end media processing pipeline
 * @details Reads TaskConfig, initializes necessary domain services (detectors, swappers, etc.),
 *          and manages the video/image processing lifecycle.
 */
class PipelineRunner {
public:
    /**
     * @brief Construct a new Pipeline Runner
     * @param app_config Global application configuration
     */
    explicit PipelineRunner(const config::AppConfig& app_config);
    ~PipelineRunner();

    PipelineRunner(const PipelineRunner&) = delete;
    PipelineRunner& operator=(const PipelineRunner&) = delete;
    PipelineRunner(PipelineRunner&&) noexcept;
    PipelineRunner& operator=(PipelineRunner&&) noexcept;

    /**
     * @brief Execute a processing task based on configuration
     * @param task_config Task-specific settings
     * @param progress_callback Function to receive completion status updates
     * @return Success or a ConfigError
     */
    [[nodiscard]] config::Result<void, config::ConfigError> Run(
        const config::TaskConfig& task_config, ProgressCallback progress_callback = nullptr);

    /**
     * @brief Abort the currently running task
     */
    void Cancel();

    /**
     * @brief Check if a task is actively processing
     */
    [[nodiscard]] bool IsRunning() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief Factory function to create a configured PipelineRunner
 * @param app_config Application configuration (model paths, inference settings, etc.)
 * @return std::unique_ptr<PipelineRunner> The created runner instance
 */
[[nodiscard]] std::unique_ptr<PipelineRunner> CreatePipelineRunner(
    const config::AppConfig& app_config);

} // namespace services::pipeline
