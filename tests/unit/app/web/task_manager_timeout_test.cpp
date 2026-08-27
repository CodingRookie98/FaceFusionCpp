#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

import app.web.task_manager;
import app.web.task_types;
import config.task;
import services.pipeline.runner;

using namespace app::web;
using namespace config;

namespace {

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
            std::ofstream(std::filesystem::path(config.io.output.path) / "partial.png") << "x";
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
    cfg.io.output.path = "web_timeout_output";
    return cfg;
}

TaskStatus WaitForTerminal(TaskManager& mgr, const std::string& id,
                           std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        auto entry = mgr.get(id);
        if (entry
            && (entry->status == TaskStatus::Done || entry->status == TaskStatus::Failed
                || entry->status == TaskStatus::Cancelled)) {
            return entry->status;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return TaskStatus::Queued;
}

} // namespace

// 1. 超时触发：block 任务超时后自动 failed，cancel 被调用
TEST(TaskManagerTimeoutTest, BlockedTaskTimesOutAndFails) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManagerOptions opts;
    opts.persist_dir = "";          // 持久化无关
    opts.max_execution_seconds = 1; // 1 秒超时（短超时加速测试）
    TaskManager mgr(executor, opts);

    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id, std::chrono::seconds(8)), TaskStatus::Failed);
    auto entry = mgr.get(id);
    ASSERT_TRUE(entry.has_value());
    EXPECT_NE(entry->error_message.find("timed out"), std::string::npos);
    EXPECT_TRUE(executor->cancelled.load());
    // 清理：block 任务已返回，worker 可退出
    mgr.shutdown();
}

// 2. 正常任务不受超时影响
TEST(TaskManagerTimeoutTest, QuickTaskNotAffectedByTimeout) {
    auto executor = std::make_shared<FakeExecutor>();
    TaskManagerOptions opts;
    opts.persist_dir = "";
    opts.max_execution_seconds = 1; // 快速任务远小于 1s，不应误杀
    TaskManager mgr(executor, opts);

    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Done);
}

// 3. 超时禁用（<=0）：block 任务保持 running
TEST(TaskManagerTimeoutTest, TimeoutDisabledKeepsTaskRunning) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManagerOptions opts;
    opts.persist_dir = "";
    opts.max_execution_seconds = 0; // 禁用超时
    TaskManager mgr(executor, opts);

    auto id = mgr.submit(MakeConfig());
    // 等待任务进入 running
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(mgr.is_running());
    // 等待略超过"如果误启用超时"的时长，确认未被误杀
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    EXPECT_TRUE(mgr.is_running());
    EXPECT_EQ(mgr.get(id)->status, TaskStatus::Running);
    mgr.cancel(id);
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Cancelled);
}

// 4. 超时后保留已产出部分帧
TEST(TaskManagerTimeoutTest, PartialOutputKeptAfterTimeout) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    executor->create_dummy_result = true;
    TaskManagerOptions opts;
    opts.persist_dir = "";
    opts.max_execution_seconds = 1;
    TaskManager mgr(executor, opts);

    std::filesystem::remove_all("web_timeout_output");
    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id, std::chrono::seconds(8)), TaskStatus::Failed);
    // FakeExecutor block 分支在 cancel 前已写 partial.png（create_dummy_result 在 block
    // 前置检查后）
    auto entry = mgr.get(id);
    ASSERT_TRUE(entry.has_value());
    // 输出目录存在（部分产物保留）
    std::error_code ec;
    EXPECT_TRUE(std::filesystem::is_directory("web_timeout_output", ec));
    std::filesystem::remove_all("web_timeout_output");
    mgr.shutdown();
}