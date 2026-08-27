#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>

import app.web.task_manager;
import app.web.task_types;
import config.parser;
import config.task;
import services.pipeline.runner;

using namespace app::web;
using namespace config;

namespace {

constexpr const char* kPersistDir = "web_persist_test";

class FakeExecutor : public ITaskExecutor {
public:
    int run(const TaskConfig& config, const services::pipeline::ProgressCallback& cb) override {
        if (block) {
            while (!cancelled.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
            return 2;
        }
        if (create_dummy_result && !config.io.output.path.empty()) {
            std::error_code ec;
            std::filesystem::create_directories(config.io.output.path, ec);
            std::ofstream(std::filesystem::path(config.io.output.path) / "result.png") << "x";
        }
        return fail_code;
    }
    void cancel() override { cancelled.store(true); }

    std::atomic<bool> cancelled{false};
    bool block = false;
    bool create_dummy_result = false;
    int fail_code = 0;
};

TaskConfig MakeConfig() {
    TaskConfig cfg;
    cfg.config_version = "1.0";
    cfg.io.source_paths = {"src.jpg"};
    cfg.io.target_paths = {"tgt.jpg"};
    cfg.io.output.path = "web_persist_output";
    return cfg;
}

TaskManagerOptions MakeOptions() {
    TaskManagerOptions opts;
    opts.persist_dir = kPersistDir;
    opts.max_execution_seconds = 3600;
    return opts;
}

void RemovePersistDir() {
    std::error_code ec;
    std::filesystem::remove_all(kPersistDir, ec);
}

TaskStatus WaitForTerminal(TaskManager& mgr, const std::string& id,
                           std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        auto entry = mgr.get(id);
        if (entry && entry->status != TaskStatus::Queued) { return entry->status; }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return TaskStatus::Queued;
}

} // namespace

// 1. submit 后生成快照文件
TEST(TaskManagerPersistTest, SubmitWritesSnapshotFile) {
    RemovePersistDir();
    auto executor = std::make_shared<FakeExecutor>();
    {
        TaskManager mgr(executor, MakeOptions());
        auto id = mgr.submit(MakeConfig());
        EXPECT_FALSE(id.empty());
        auto snapshot = std::filesystem::path(kPersistDir) / (id + ".json");
        EXPECT_TRUE(std::filesystem::exists(snapshot));
        // 内容包含任务配置
        std::ifstream in(snapshot);
        std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("source_paths"), std::string::npos);
    }
    RemovePersistDir();
}

// 2. 默认 persist_dir 空 → 不产生快照（旧行为保持）
TEST(TaskManagerPersistTest, DefaultNoPersistDirProducesNoSnapshots) {
    RemovePersistDir();
    auto executor = std::make_shared<FakeExecutor>();
    {
        TaskManager mgr(executor); // 默认构造
        auto id = mgr.submit(MakeConfig());
        EXPECT_FALSE(id.empty());
        EXPECT_FALSE(std::filesystem::exists(std::filesystem::path(kPersistDir)));
    }
    RemovePersistDir();
}

// 3. 模拟重启：queued 任务恢复排队并可执行
TEST(TaskManagerPersistTest, QueuedTaskRestoredAndExecutes) {
    RemovePersistDir();
    std::string task_id;
    {
        auto executor = std::make_shared<FakeExecutor>();
        executor->block = true; // 保持第一个任务 running，让后续任务 queued
        TaskManager mgr(executor, MakeOptions());
        auto first = mgr.submit(MakeConfig());
        // 等待 worker 占用
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        auto queued = mgr.submit(MakeConfig());
        task_id = queued;
        EXPECT_EQ(mgr.get(queued)->status, TaskStatus::Queued);
        mgr.cancel(first);
        // mgr 析构 → shutdown
    }
    // 模拟重启：新 TaskManager 同 persist_dir
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        auto restored = mgr.get(task_id);
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->status, TaskStatus::Queued);
        // 恢复后任务可执行至 done
        EXPECT_EQ(WaitForTerminal(mgr, task_id), TaskStatus::Done);
    }
    RemovePersistDir();
}

// 4. running 任务重启后标记 failed
TEST(TaskManagerPersistTest, RunningTaskRestoredAsFailed) {
    RemovePersistDir();
    std::error_code ec;
    std::filesystem::create_directories(kPersistDir, ec);

    // 直接构造 running 态快照，精确模拟崩溃瞬间磁盘状态
    auto cfg = MakeConfig();
    auto cfg_json = config::SerializeTaskConfig(cfg);
    ASSERT_TRUE(cfg_json.is_ok());
    nlohmann::json snap = {{"id", "crashed_task"},
                           {"status", "running"},
                           {"config", cfg_json.value()},
                           {"priority", 0},
                           {"created_at", 1234567890}};
    {
        std::ofstream out(std::filesystem::path(kPersistDir) / "crashed_task.json");
        out << snap.dump();
    }

    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        auto restored = mgr.get("crashed_task");
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->status, TaskStatus::Failed);
        EXPECT_NE(restored->error_message.find("restart"), std::string::npos);
    }
    RemovePersistDir();
}

// 5. 终态任务保留为历史，不重新执行
TEST(TaskManagerPersistTest, TerminalTaskKeptAsHistory) {
    RemovePersistDir();
    std::string task_id;
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        task_id = mgr.submit(MakeConfig());
        EXPECT_EQ(WaitForTerminal(mgr, task_id), TaskStatus::Done);
    }
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        auto restored = mgr.get(task_id);
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->status, TaskStatus::Done);
        EXPECT_FALSE(mgr.is_running()); // 未重新执行
    }
    RemovePersistDir();
}

// 6. 损坏快照被跳过，不阻塞恢复
TEST(TaskManagerPersistTest, CorruptSnapshotSkipped) {
    RemovePersistDir();
    std::error_code ec;
    std::filesystem::create_directories(kPersistDir, ec);
    std::ofstream(std::filesystem::path(kPersistDir) / "corrupt.json") << "{not valid json";
    std::string good_id;
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        good_id = mgr.submit(MakeConfig());
        EXPECT_EQ(WaitForTerminal(mgr, good_id), TaskStatus::Done);
    }
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        // 正常任务仍恢复
        auto restored = mgr.get(good_id);
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->status, TaskStatus::Done);
    }
    RemovePersistDir();
}

// 7. cancel/set_priority 后快照同步更新
TEST(TaskManagerPersistTest, SnapshotUpdatesOnCancelAndPriorityChange) {
    RemovePersistDir();
    std::string task_id;
    {
        auto executor = std::make_shared<FakeExecutor>();
        executor->block = true;
        TaskManager mgr(executor, MakeOptions());
        auto first = mgr.submit(MakeConfig());
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        task_id = mgr.submit(MakeConfig());
        EXPECT_TRUE(mgr.set_priority(task_id, 7));
        mgr.cancel(task_id);
        EXPECT_EQ(WaitForTerminal(mgr, task_id), TaskStatus::Cancelled);
    }
    {
        auto executor = std::make_shared<FakeExecutor>();
        TaskManager mgr(executor, MakeOptions());
        auto restored = mgr.get(task_id);
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->status, TaskStatus::Cancelled);
    }
    RemovePersistDir();
}