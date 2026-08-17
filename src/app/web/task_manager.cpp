module;
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <limits>
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

    /// Pick the highest-priority queued task (FIFO among equal priorities)
    static std::string pick_next(const std::map<std::string, TaskEntry>& tasks,
                                 std::deque<std::string>& queue) {
        auto best = queue.end();
        int best_priority = std::numeric_limits<int>::min();
        for (auto it = queue.begin(); it != queue.end(); ++it) {
            auto eit = tasks.find(*it);
            if (eit == tasks.end() || eit->second.status != TaskStatus::Queued) {
                continue;
            }
            if (best == queue.end() || eit->second.priority > best_priority) {
                best = it;
                best_priority = eit->second.priority;
            }
        }
        if (best == queue.end()) { return {}; }
        std::string id = *best;
        queue.erase(best);
        return id;
    }

    void worker_loop() {
        for (;;) {
            std::string task_id;
            {
                std::unique_lock lock(mutex);
                // Wait until there is a runnable (queued) task or shutdown.
                // Cancelled tasks may stay in the queue; they must not wake us.
                stop_cv.wait(lock, [this] {
                    if (stopping) { return true; }
                    for (const auto& qid : queue) {
                        auto it = tasks.find(qid);
                        if (it != tasks.end() && it->second.status == TaskStatus::Queued) {
                            return true;
                        }
                    }
                    return false;
                });
                if (stopping) { return; }
                task_id = pick_next(tasks, queue);
                if (task_id.empty()) { continue; }
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
                    // Cancelled wins over the executor result (user cancellation)
                    if (it->second.status != TaskStatus::Cancelled) {
                        if (result_code == 0) {
                            it->second.status = TaskStatus::Done;
                            it->second.result_files = collect_result_files(it->second.config);
                        } else {
                            it->second.status = TaskStatus::Failed;
                            it->second.error_message =
                                "Task failed with error code " + std::to_string(result_code);
                        }
                    }
                    if (status_listener) {
                        status_listener(task_id, it->second.status, it->second.error_message);
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
    StatusListener status_listener;
    std::thread worker;
};

TaskManager::TaskManager(std::shared_ptr<ITaskExecutor> executor)
    : m_impl(std::make_unique<Impl>(std::move(executor))) {}

TaskManager::~TaskManager() = default;

std::string TaskManager::submit(config::TaskConfig config, int priority) {
    std::string uuid = foundation::infrastructure::core_utils::random::generate_uuid();
    std::replace(uuid.begin(), uuid.end(), '-', '_');

    TaskEntry entry;
    entry.id = uuid;
    entry.config = std::move(config);
    entry.created_at = std::chrono::system_clock::now();
    entry.priority = priority;

    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->tasks[uuid] = std::move(entry);
        m_impl->queue.push_back(uuid);
    }
    m_impl->stop_cv.notify_all();
    return uuid;
}

bool TaskManager::set_priority(const std::string& id, int priority) {
    std::lock_guard lock(m_impl->mutex);
    auto it = m_impl->tasks.find(id);
    if (it == m_impl->tasks.end() || it->second.status != TaskStatus::Queued) {
        return false;
    }
    it->second.priority = priority;
    return true;
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
        s.priority = entry.priority;
        out.push_back(std::move(s));
    }
    // Queue position: 1-based rank among queued tasks by (priority desc, created_at asc)
    std::vector<TaskSummary*> queued;
    for (auto& s : out) {
        if (s.status == TaskStatus::Queued) { queued.push_back(&s); }
    }
    std::sort(queued.begin(), queued.end(), [](const TaskSummary* a, const TaskSummary* b) {
        if (a->priority != b->priority) { return a->priority > b->priority; }
        return a->created_at < b->created_at;
    });
    for (std::size_t i = 0; i < queued.size(); ++i) { queued[i]->queue_position = static_cast<int>(i + 1); }

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

void TaskManager::set_status_listener(StatusListener listener) {
    std::lock_guard lock(m_impl->mutex);
    m_impl->status_listener = std::move(listener);
}

void TaskManager::shutdown() { m_impl->shutdown(); }

} // namespace app::web
