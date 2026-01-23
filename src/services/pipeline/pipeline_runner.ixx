/**
 * @file pipeline_runner.ixx
 * @brief 配置驱动的 Pipeline 执行器接口
 *
 * 将 TaskConfig 配置与 Domain Pipeline 系统集成，
 * 提供基于配置的任务执行能力。
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
import config.parser; // For ValidateTaskConfig

export namespace services::pipeline {

/**
 * @brief 任务进度信息 (帧级别)
 */
struct TaskProgress {
    std::string task_id;      ///< 任务ID
    size_t current_frame;     ///< 当前处理的帧序号
    size_t total_frames;      ///< 总帧数 (0 表示未知)
    std::string current_step; ///< 当前执行的 step 名称
    double fps;               ///< 当前处理速度 (帧/秒)
};

/**
 * @brief 进度回调函数类型
 */
using ProgressCallback = std::function<void(const TaskProgress&)>;

/**
 * @brief Pipeline 执行器
 *
 * 根据 TaskConfig 配置创建并执行处理流水线。
 */
class PipelineRunner {
public:
    explicit PipelineRunner(const config::AppConfig& app_config);
    ~PipelineRunner();

    PipelineRunner(const PipelineRunner&) = delete;
    PipelineRunner& operator=(const PipelineRunner&) = delete;
    PipelineRunner(PipelineRunner&&) noexcept;
    PipelineRunner& operator=(PipelineRunner&&) noexcept;

    /**
     * @brief 执行任务
     *
     * @param task_config 任务配置
     * @param progress_callback 进度回调 (可选)
     * @return Result<void> 成功返回 Ok，失败返回错误
     */
    [[nodiscard]] config::Result<void, config::ConfigError> Run(
        const config::TaskConfig& task_config, ProgressCallback progress_callback = nullptr);

    /**
     * @brief 取消正在执行的任务
     */
    void Cancel();

    /**
     * @brief 检查是否正在运行
     */
    [[nodiscard]] bool IsRunning() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * @brief 创建 PipelineRunner 实例
 *
 * @param app_config 应用配置 (模型路径、推理设置等)
 * @return unique_ptr<PipelineRunner> Runner 实例
 */
[[nodiscard]] std::unique_ptr<PipelineRunner> CreatePipelineRunner(
    const config::AppConfig& app_config);

} // namespace services::pipeline
