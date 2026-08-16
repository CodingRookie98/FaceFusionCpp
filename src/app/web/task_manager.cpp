module;
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

module app.web.task_manager;

import foundation.infrastructure.core_utils;

namespace app::web {

struct TaskManager::Impl {
    explicit Impl(std::shared_ptr<ITaskExecutor> ex) : executor(std::move(ex)) {
        worker = std::thread([this] { worker_loop(); });
    }

    ~Impl() { shutdown(); }

    void worker_loop() {
        for (;;) {
            std::string task_id;
            {
                std::unique_lock lock(mutex);
                stop_cv.wait(lock, [this] { return !queue.empty() || stopping; });
                if (stopping && queue.empty()) { return; }
                task_id = queue.front();
                queue.pop_front();
                auto it = tasks.find(task_id);
                if (it == tasks.end() || it->second.status != TaskStatus::Queued) {
                    continue; // cancelled while queued
                }
                it->second.status = TaskStatus::Running;
                running_id = task_id;
            }

            int result_code = 0;
            {
                auto it = tasks.find(task_id);
                if (it == tasks.end()) { continue; }
                // Run with progress callback wiring (defensive: executor must not throw)
                try {
                    result_code = executor->run(
                        it->second.config,
                        [this, task_id](const services::pipeline::TaskProgress& p) {
                            TaskProgress snap;
                            snap.current_frame = p.current_frame;
                            snap.total_frames = p.total_frames;
                            snap.fps = p.fps;
                            std::lock_guard lock(mutex);
                            auto it = tasks.find(task_id);
                            if (it == tasks.end()) { return; }
                            it->second.progress = snap;
                            if (listener) { listener(task_id, snap); }
                        });
                } catch (const std::exception& e) {
                    result_code = 1;
                    std::lock_guard lock(mutex);
                    auto it2 = tasks.find(task_id);
                    if (it2 != tasks.end()) { it2->second.error_message = e.what(); }
                }
            }

            {
                std::lock_guard lock(mutex);
                auto it = tasks.find(task_id);
                if (it != tasks.end()) {
                    if (result_code == 0) {
                        it->second.status = TaskStatus::Done;
                        it->second.result_files = collect_result_files(it->second.config);
                    } else {
                        it->second.status = TaskStatus::Failed;
                        it->second.error_message =
                            "Task failed with error code " + std::to_string(result_code);
                    }
                }
                running_id.clear();
            }
        }
    }

    std::vector<std::string> collect_result_files(const config::TaskConfig& config) {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
        std::error_code ec;
        fs::path out_dir(config.io.output.path);
        if (!fs::is_directory(out_dir, ec)) { return files; }
        for (const auto& entry : fs::directory_iterator(out_dir, ec)) {
            if (entry.is_regular_file(ec)) { files.push_back(entry.path().filename().string()); }
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    void shutdown() {
        {
            std::lock_guard lock(mutex);
            if (stopping) { return; }
            stopping = true;
            // Interrupt the running task so the worker thread can exit promptly
            if (!running_id.empty()) { executor->cancel(); }
        }
        stop_cv.notify_all();
        if (worker.joinable()) { worker.join(); }
    }

    std::shared_ptr<ITaskExecutor> executor;
    std::map<std::string, TaskEntry> tasks;
    std::deque<std::string> queue;
    std::string running_id;
    mutable std::mutex mutex;
    std::condition_variable stop_cv;
    bool stopping = false;
    ProgressListener listener;
    std::thread worker;
};

TaskManager::TaskManager(std::shared_ptr<ITaskExecutor> executor)
    : m_impl(std::make_unique<Impl>(std::move(executor))) {}

TaskManager::~TaskManager() = default;

std::string TaskManager::submit(config::TaskConfig config) {
    std::string uuid = foundation::infrastructure::core_utils::random::generate_uuid();
    std::replace(uuid.begin(), uuid.end(), '-', '_');

    TaskEntry entry;
    entry.id = uuid;
    entry.config = std::move(config);
    entry.created_at = std::chrono::system_clock::now();

    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->tasks[uuid] = std::move(entry);
        m_impl->queue.push_back(uuid);
    }
    m_impl->stop_cv.notify_all();
    return uuid;
}

bool TaskManager::cancel(const std::string& id) {
    std::lock_guard lock(m_impl->mutex);
    auto it = m_impl->tasks.find(id);
    if (it == m_impl->tasks.end()) { return false; }
    switch (it->second.status) {
    case TaskStatus::Queued:
        it->second.status = TaskStatus::Cancelled;
        return true;
    case TaskStatus::Running:
        it->second.status = TaskStatus::Cancelled;
        m_impl->executor->cancel();
        return true;
    default:
        return true; // already terminal; no-op
    }
}

std::optional<TaskEntry> TaskManager::get(const std::string& id) const {
    std::lock_guard lock(m_impl->mutex);
    auto it = m_impl->tasks.find(id);
    if (it == m_impl->tasks.end()) { return std::nullopt; }
    return it->second;
}

std::vector<TaskSummary> TaskManager::list() const {
    std::lock_guard lock(m_impl->mutex);
    std::vector<TaskSummary> out;
    out.reserve(m_impl->tasks.size());
    for (const auto& [id, entry] : m_impl->tasks) {
        TaskSummary s;
        s.id = id;
        s.status = entry.status;
        s.progress = entry.progress;
        s.error_message = entry.error_message;
        s.media_count = entry.config.io.target_paths.size();
        s.created_at = entry.created_at;
        out.push_back(std::move(s));
    }
    // Newest first (stable across clock granularity)
    std::stable_sort(out.begin(), out.end(), [](const TaskSummary& a, const TaskSummary& b) {
        return a.created_at > b.created_at;
    });
    return out;
}

bool TaskManager::is_running() const {
    std::lock_guard lock(m_impl->mutex);
    return !m_impl->running_id.empty();
}

void TaskManager::set_progress_listener(ProgressListener listener) {
    std::lock_guard lock(m_impl->mutex);
    m_impl->listener = std::move(listener);
}

void TaskManager::shutdown() { m_impl->shutdown(); }

} // namespace app::web
