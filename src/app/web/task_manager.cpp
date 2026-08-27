module;
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <variant>
#include <nlohmann/json.hpp>

module app.web.task_manager;

import config.parser;
import foundation.infrastructure.core_utils;
import foundation.infrastructure.logger;

namespace app::web {

using Logger = foundation::infrastructure::logger::Logger;
using json = nlohmann::json;

namespace {

std::string task_status_to_string(TaskStatus status) {
    switch (status) {
    case TaskStatus::Queued: return "queued";
    case TaskStatus::Running: return "running";
    case TaskStatus::Done: return "done";
    case TaskStatus::Failed: return "failed";
    case TaskStatus::Cancelled: return "cancelled";
    }
    return "unknown";
}

TaskStatus task_status_from_string(const std::string& s) {
    if (s == "queued") return TaskStatus::Queued;
    if (s == "running") return TaskStatus::Running;
    if (s == "done") return TaskStatus::Done;
    if (s == "failed") return TaskStatus::Failed;
    if (s == "cancelled") return TaskStatus::Cancelled;
    return TaskStatus::Failed;
}

json progress_to_json(const TaskProgress& p) {
    return json{
        {"current_frame", p.current_frame}, {"total_frames", p.total_frames}, {"fps", p.fps}};
}

TaskProgress progress_from_json(const json& j) {
    TaskProgress p;
    if (j.contains("current_frame") && j["current_frame"].is_number()) {
        p.current_frame = j["current_frame"].get<std::size_t>();
    }
    if (j.contains("total_frames") && j["total_frames"].is_number()) {
        p.total_frames = j["total_frames"].get<std::size_t>();
    }
    if (j.contains("fps") && j["fps"].is_number()) { p.fps = j["fps"].get<double>(); }
    return p;
}

json task_entry_to_json(const TaskEntry& e) {
    return json{
        {"id", e.id},
        {"status", task_status_to_string(e.status)},
        {"config", config::SerializeTaskConfig(e.config).value_or(json::object())},
        {"progress", progress_to_json(e.progress)},
        {"error_message", e.error_message},
        {"result_files", e.result_files},
        {"created_at",
         std::chrono::duration_cast<std::chrono::seconds>(e.created_at.time_since_epoch()).count()},
        {"priority", e.priority}};
}

} // namespace

struct TaskManager::Impl {
    explicit Impl(std::shared_ptr<ITaskExecutor> ex, TaskManagerOptions opts) :
        executor(std::move(ex)), persist_dir(std::move(opts.persist_dir)),
        max_execution_seconds(opts.max_execution_seconds) {
        load_snapshots();
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
            if (eit == tasks.end() || eit->second.status != TaskStatus::Queued) { continue; }
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
                save_snapshot(it->second);
                Logger::get_instance()->info(std::format(
                    "[TaskManager] Task {} picked from queue (priority={}), status set to Running",
                    task_id, it->second.priority));
            }

            config::TaskConfig exec_config;
            {
                std::lock_guard lock(mutex);
                auto it = tasks.find(task_id);
                if (it == tasks.end()) { continue; }
                exec_config = it->second.config;
            }

            int result_code = 0;
            std::string err_msg;
            std::atomic<bool> run_finished{false};
            bool timed_out = false;

            std::thread exec_thread([this, task_id, &exec_config, &result_code, &err_msg,
                                     &run_finished] {
                try {
                    Logger::get_instance()->info(std::format(
                        "[TaskManager] Task {} started execution (targets={}, sources={})", task_id,
                        exec_config.io.target_paths.size(), exec_config.io.source_paths.size()));
                    result_code = executor->run(
                        exec_config,
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
                        },
                        err_msg);
                } catch (const std::exception& e) {
                    result_code = 1;
                    err_msg = e.what();
                }
                run_finished.store(true);
                stop_cv.notify_all();
            });

            {
                std::unique_lock lock(mutex);
                auto started = std::chrono::steady_clock::now();
                while (!run_finished.load()) {
                    if (max_execution_seconds > 0) {
                        auto remaining = std::chrono::seconds(max_execution_seconds)
                                       - (std::chrono::steady_clock::now() - started);
                        if (remaining <= std::chrono::seconds(0)) {
                            timed_out = true;
                            executor->cancel();
                            break;
                        }
                        stop_cv.wait_for(lock, remaining);
                    } else {
                        stop_cv.wait(lock, [this, &run_finished] {
                            return run_finished.load() || stopping;
                        });
                        if (stopping && !run_finished.load()) { executor->cancel(); }
                    }
                }
            }
            if (exec_thread.joinable()) { exec_thread.join(); }

            {
                std::lock_guard lock(mutex);
                auto it = tasks.find(task_id);
                if (it != tasks.end()) {
                    if (timed_out) {
                        it->second.status = TaskStatus::Failed;
                        it->second.error_message =
                            "Task timed out after " + std::to_string(max_execution_seconds) + "s";
                        Logger::get_instance()->error(
                            std::format("[TaskManager] Task {} timed out after {}s", task_id,
                                        max_execution_seconds));
                    } else if (it->second.status != TaskStatus::Cancelled) {
                        if (result_code == 0) {
                            it->second.status = TaskStatus::Done;
                            it->second.result_files = collect_result_files(it->second.config);
                            Logger::get_instance()->info(std::format(
                                "[TaskManager] Task {} completed successfully with {} result file(s)",
                                task_id, it->second.result_files.size()));
                        } else {
                            it->second.status = TaskStatus::Failed;
                            if (!err_msg.empty()) {
                                it->second.error_message = err_msg;
                            } else if (it->second.error_message.empty()) {
                                it->second.error_message =
                                    "Task failed with error code " + std::to_string(result_code);
                            }
                            Logger::get_instance()->error(
                                std::format("[TaskManager] Task {} failed with code {}: {}",
                                            task_id, result_code, it->second.error_message));
                        }
                    } else {
                        Logger::get_instance()->warn(
                            std::format("[TaskManager] Task {} ended in Cancelled state", task_id));
                    }
                    if (status_listener) {
                        status_listener(task_id, it->second.status, it->second.error_message);
                    }
                    save_snapshot(it->second);
                }
                running_id.clear();
            }
        }
    }

    void save_snapshot(const TaskEntry& entry) {
        if (persist_dir.empty()) { return; }
        std::error_code ec;
        std::filesystem::create_directories(persist_dir, ec);
        auto path = std::filesystem::path(persist_dir) / (entry.id + ".json");
        std::ofstream out(path, std::ios::trunc);
        if (!out) {
            Logger::get_instance()->warn(
                std::format("[TaskManager] Failed to write snapshot for task {}", entry.id));
            return;
        }
        out << task_entry_to_json(entry).dump(2);
    }

    void remove_snapshot(const std::string& id) {
        if (persist_dir.empty()) { return; }
        std::error_code ec;
        std::filesystem::remove(std::filesystem::path(persist_dir) / (id + ".json"), ec);
    }

    void load_snapshots() {
        if (persist_dir.empty()) { return; }
        std::error_code ec;
        if (!std::filesystem::is_directory(persist_dir, ec)) { return; }

        for (const auto& entry : std::filesystem::directory_iterator(persist_dir, ec)) {
            if (!entry.is_regular_file(ec) || entry.path().extension() != ".json") { continue; }
            std::ifstream in(entry.path());
            if (!in) { continue; }
            std::string content((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
            json j;
            try {
                j = json::parse(content);
            } catch (const std::exception& e) {
                Logger::get_instance()->warn(
                    std::format("[TaskManager] Skipping corrupt snapshot {}: {}",
                                entry.path().string(), e.what()));
                continue;
            }

            TaskEntry task;
            task.id = j.contains("id") && j["id"].is_string() ? j["id"].get<std::string>() : "";
            if (task.id.empty()) {
                Logger::get_instance()->warn(std::format(
                    "[TaskManager] Skipping snapshot {} (missing id)", entry.path().string()));
                continue;
            }
            task.status = task_status_from_string(j.contains("status") && j["status"].is_string() ?
                                                      j["status"].get<std::string>() :
                                                      "failed");
            if (j.contains("config") && j["config"].is_object()) {
                auto cfg = config::DeserializeTaskConfig(j["config"]);
                if (cfg.is_ok()) { task.config = cfg.value(); }
            }
            if (j.contains("progress") && j["progress"].is_object()) {
                task.progress = progress_from_json(j["progress"]);
            }
            task.error_message = j.contains("error_message") && j["error_message"].is_string() ?
                                     j["error_message"].get<std::string>() :
                                     std::string{};
            if (j.contains("result_files") && j["result_files"].is_array()) {
                for (const auto& f : j["result_files"]) {
                    if (f.is_string()) { task.result_files.push_back(f.get<std::string>()); }
                }
            }
            if (j.contains("created_at") && j["created_at"].is_number()) {
                task.created_at = std::chrono::system_clock::time_point(
                    std::chrono::seconds(j["created_at"].get<std::int64_t>()));
            }
            task.priority =
                j.contains("priority") && j["priority"].is_number() ? j["priority"].get<int>() : 0;

            // 恢复语义：Queued 重新入队；Running 如实标记 failed；终态保留为历史
            if (task.status == TaskStatus::Queued) {
                tasks[task.id] = task;
                queue.push_back(task.id);
                Logger::get_instance()->info(std::format(
                    "[TaskManager] Restored queued task {} (priority={})", task.id, task.priority));
            } else if (task.status == TaskStatus::Running) {
                task.status = TaskStatus::Failed;
                task.error_message = "Task interrupted by service restart";
                tasks[task.id] = task;
                save_snapshot(task);
                Logger::get_instance()->warn(std::format(
                    "[TaskManager] Task {} was running at restart; marked failed", task.id));
            } else {
                tasks[task.id] = task;
                Logger::get_instance()->info(
                    std::format("[TaskManager] Restored terminal task {} ({})", task.id,
                                task_status_to_string(task.status)));
            }
        }
    }

    std::vector<std::string> collect_result_files(const config::TaskConfig& config) {
        namespace fs = std::filesystem;
        std::vector<std::string> files;
        std::error_code ec;
        fs::path out_dir(config.io.output.path);
        if (!fs::is_directory(out_dir, ec)) { return files; }

        std::vector<std::string> all_files;
        for (const auto& entry : fs::directory_iterator(out_dir, ec)) {
            if (entry.is_regular_file(ec)) {
                all_files.push_back(entry.path().filename().string());
            }
        }
        std::sort(all_files.begin(), all_files.end());

        // First attempt: match specific target paths stems
        std::vector<std::string> target_stems;
        for (const auto& tp : config.io.target_paths) {
            fs::path p(tp);
            if (!p.stem().empty()) { target_stems.push_back(p.stem().string()); }
        }

        if (!target_stems.empty()) {
            for (const auto& stem : target_stems) {
                for (const auto& f : all_files) {
                    if (f.find(stem) != std::string::npos
                        && std::find(files.begin(), files.end(), f) == files.end()) {
                        files.push_back(f);
                    }
                }
            }
        }

        // Fallback: if no target stem matches, return all files in output dir
        if (files.empty()) { files = std::move(all_files); }

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
    std::string persist_dir;
    int max_execution_seconds = 3600;
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

TaskManager::TaskManager(std::shared_ptr<ITaskExecutor> executor, TaskManagerOptions options) :
    m_impl(std::make_unique<Impl>(std::move(executor), std::move(options))) {}

TaskManager::~TaskManager() = default;

std::string TaskManager::submit(config::TaskConfig config, int priority) {
    std::string uuid = foundation::infrastructure::core_utils::random::generate_uuid();
    std::replace(uuid.begin(), uuid.end(), '-', '_');

    // Partition output directory per task to guarantee physical isolation
    std::filesystem::path base_out(config.io.output.path.empty() ? "./output" :
                                                                   config.io.output.path);
    if (base_out.filename().string() != uuid) {
        config.io.output.path = (base_out / uuid).string();
    }
    std::error_code ec;
    std::filesystem::create_directories(config.io.output.path, ec);

    TaskEntry entry;
    entry.id = uuid;
    entry.config = std::move(config);
    entry.created_at = std::chrono::system_clock::now();
    entry.priority = priority;

    Logger::get_instance()->info(std::format(
        "[TaskManager] Submitting task: id={}, priority={}, output={}, targets={}, sources={}",
        uuid, priority, entry.config.io.output.path, entry.config.io.target_paths.size(),
        entry.config.io.source_paths.size()));

    {
        std::lock_guard lock(m_impl->mutex);
        m_impl->tasks[uuid] = std::move(entry);
        m_impl->queue.push_back(uuid);
        auto it = m_impl->tasks.find(uuid);
        if (it != m_impl->tasks.end()) { m_impl->save_snapshot(it->second); }
    }
    m_impl->stop_cv.notify_all();
    return uuid;
}

std::shared_ptr<ITaskExecutor> TaskManager::get_executor() const {
    return m_impl->executor;
}

bool TaskManager::set_priority(const std::string& id, int priority) {
    std::lock_guard lock(m_impl->mutex);
    auto it = m_impl->tasks.find(id);
    if (it == m_impl->tasks.end() || it->second.status != TaskStatus::Queued) { return false; }
    int old_priority = it->second.priority;
    it->second.priority = priority;
    Logger::get_instance()->info(std::format("[TaskManager] Task {} priority changed from {} to {}",
                                             id, old_priority, priority));
    m_impl->save_snapshot(it->second);
    return true;
}

bool TaskManager::cancel(const std::string& id) {
    std::lock_guard lock(m_impl->mutex);
    auto it = m_impl->tasks.find(id);
    if (it == m_impl->tasks.end()) {
        Logger::get_instance()->warn(
            std::format("[TaskManager] Cancel failed: task {} not found", id));
        return false;
    }
    Logger::get_instance()->warn(
        std::format("[TaskManager] Task {} cancel requested (current status: {})", id,
                    status_to_string(it->second.status)));
    switch (it->second.status) {
    case TaskStatus::Queued:
        it->second.status = TaskStatus::Cancelled;
        m_impl->save_snapshot(it->second);
        return true;
    case TaskStatus::Running:
        it->second.status = TaskStatus::Cancelled;
        m_impl->executor->cancel();
        return true;
    default: return true; // already terminal; no-op
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
    for (std::size_t i = 0; i < queued.size(); ++i) {
        queued[i]->queue_position = static_cast<int>(i + 1);
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

void TaskManager::set_status_listener(StatusListener listener) {
    std::lock_guard lock(m_impl->mutex);
    m_impl->status_listener = std::move(listener);
}

void TaskManager::shutdown() {
    m_impl->shutdown();
}

} // namespace app::web
