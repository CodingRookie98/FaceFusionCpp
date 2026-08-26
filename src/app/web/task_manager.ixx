/**
 * @file task_manager.ixx
 * @brief Web task registry with a single serial worker thread
 * @details Executor is injectable so unit/integration tests can use fakes;
 *          production wires it to services::pipeline::PipelineRunner.
 */
module;

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

export module app.web.task_manager;

import app.web.task_types;
import config.task;
import services.pipeline.runner;

export namespace app::web {

/// Injectable task executor (see Global Constraints: fakes in tests)
class ITaskExecutor {
public:
    virtual ~ITaskExecutor() = default;
    /// Run one task; returns 0 on success, non-zero error code on failure
    virtual int run(const config::TaskConfig& config,
                    const services::pipeline::ProgressCallback& progress) {
        std::string err;
        return run(config, progress, err);
    }
    virtual int run(const config::TaskConfig& config,
                    const services::pipeline::ProgressCallback& progress,
                    std::string& error_message) {
        return run(config, progress);
    }
    /// Abort the currently running task (no-op when idle)
    virtual void cancel() = 0;
};

/// Progress listener (e.g. WebSocket broadcaster)
using ProgressListener =
    std::function<void(const std::string& task_id, const TaskProgress& progress)>;

/// Status change listener (called when a task reaches a terminal state)
using StatusListener = std::function<void(const std::string& task_id, TaskStatus status,
                                          const std::string& error_message)>;

/// Task registry + serial worker thread
class TaskManager {
public:
    explicit TaskManager(std::shared_ptr<ITaskExecutor> executor);
    ~TaskManager();
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    /// Enqueue a task; returns the new task id
    /// @param priority higher value = scheduled earlier (FIFO among equals)
    std::string submit(config::TaskConfig config, int priority = 0);

    /// Change the priority of a queued task (no-op for running/terminal)
    /// @return false if the task does not exist or is not queued
    bool set_priority(const std::string& id, int priority);

    /// Cancel a queued/running task; returns false if task not found
    bool cancel(const std::string& id);

    /// Fetch a task entry by id
    std::optional<TaskEntry> get(const std::string& id) const;

    /// List task summaries (newest first)
    std::vector<TaskSummary> list() const;

    /// Whether a task is currently executing
    bool is_running() const;

    /// Register a listener for progress updates (called from worker thread)
    void set_progress_listener(ProgressListener listener);

    /// Register a listener for terminal status changes (called from worker thread)
    void set_status_listener(StatusListener listener);

    /// Stop the worker thread (called on server shutdown)
    void shutdown();

    /// Get underlying task executor
    std::shared_ptr<ITaskExecutor> get_executor() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace app::web
