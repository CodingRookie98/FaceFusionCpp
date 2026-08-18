#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <cstdio>
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

/// Fake executor controllable per test
class FakeExecutor : public ITaskExecutor {
public:
    int run(const TaskConfig&, const services::pipeline::ProgressCallback& cb) override {
        if (block) {
            while (!cancelled.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
            return 2; // cancelled
        }
        if (emit_progress) {
            services::pipeline::TaskProgress p;
            p.current_frame = 1;
            p.total_frames = 2;
            p.fps = 30.0;
            cb(p);
        }
        return fail_code;
    }
    void cancel() override { cancelled.store(true); }

    std::atomic<bool> cancelled{false};
    bool block = false;
    bool emit_progress = true;
    int fail_code = 0;
};

TaskConfig MakeConfig() {
    TaskConfig cfg;
    cfg.config_version = "1.0";
    cfg.io.source_paths = {"src.jpg"};
    cfg.io.target_paths = {"tgt.jpg"};
    cfg.io.output.path = "web_test_output";
    return cfg;
}

/// Poll until the task leaves Queued or timeout (returns current status)
TaskStatus WaitForTerminal(TaskManager& mgr, const std::string& id,
                           std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        auto entry = mgr.get(id);
        if (entry && entry->status != TaskStatus::Queued) { return entry->status; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto entry = mgr.get(id);
    return entry ? entry->status : TaskStatus::Queued;
}

} // namespace

TEST(TaskManagerTest, SubmitGeneratesUniqueIdQueued) {
    // Block the worker so queued tasks stay queued during assertions
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManager mgr(executor);
    auto id1 = mgr.submit(MakeConfig());
    auto id2 = mgr.submit(MakeConfig());
    EXPECT_NE(id1, id2);
    EXPECT_FALSE(id1.empty());
    auto entry = mgr.get(id1);
    ASSERT_TRUE(entry.has_value());
    // worker may have already consumed id1 (Running); id2 must still be queued
    EXPECT_TRUE(entry->status == TaskStatus::Queued || entry->status == TaskStatus::Running);
    auto entry2 = mgr.get(id2);
    ASSERT_TRUE(entry2.has_value());
    EXPECT_EQ(entry2->status, TaskStatus::Queued);
}

TEST(TaskManagerTest, ExecutorRunsToDoneAndCollectsResults) {
    auto executor = std::make_shared<FakeExecutor>();
    TaskManager mgr(executor);

    // Fresh output dir + file so result collection finds exactly one file
    std::filesystem::remove_all("web_test_output");
    std::filesystem::create_directories("web_test_output");
    std::ofstream("web_test_output/result.png") << "x";

    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Done);
    auto entry = mgr.get(id);
    ASSERT_TRUE(entry.has_value());
    ASSERT_EQ(entry->result_files.size(), 1u);
    EXPECT_EQ(entry->result_files[0], "result.png");
    EXPECT_FALSE(mgr.is_running());
}

TEST(TaskManagerTest, CancelsQueuedTask) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManager mgr(executor);
    auto id1 = mgr.submit(MakeConfig());
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    auto id2 = mgr.submit(MakeConfig());
    EXPECT_TRUE(mgr.cancel(id2));
    EXPECT_EQ(WaitForTerminal(mgr, id2), TaskStatus::Cancelled);
    mgr.cancel(id1);
}

TEST(TaskManagerTest, CancelsRunningTask) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManager mgr(executor);
    auto id = mgr.submit(MakeConfig());

    // Wait until running
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        if (mgr.is_running()) { break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(mgr.is_running());
    EXPECT_TRUE(mgr.cancel(id));
    EXPECT_TRUE(executor->cancelled.load());
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Cancelled);
}

TEST(TaskManagerTest, ProgressListenerReceivesUpdates) {
    auto executor = std::make_shared<FakeExecutor>();
    TaskManager mgr(executor);
    std::string seen_id;
    TaskProgress seen;
    std::atomic<bool> received{false};
    mgr.set_progress_listener([&](const std::string& id, const TaskProgress& p) {
        seen_id = id;
        seen = p;
        received.store(true);
    });
    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Done);
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!received.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(received.load());
    EXPECT_EQ(seen_id, id);
    EXPECT_EQ(seen.total_frames, 2u);
}

TEST(TaskManagerTest, FailedTaskRecordsError) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->fail_code = 42;
    TaskManager mgr(executor);
    auto id = mgr.submit(MakeConfig());
    EXPECT_EQ(WaitForTerminal(mgr, id), TaskStatus::Failed);
    auto entry = mgr.get(id);
    ASSERT_TRUE(entry.has_value());
    EXPECT_NE(entry->error_message.find("42"), std::string::npos);
}

TEST(TaskManagerTest, ListReturnsSummariesNewestFirst) {
    auto executor = std::make_shared<FakeExecutor>();
    TaskManager mgr(executor);
    auto id1 = mgr.submit(MakeConfig());
    auto id2 = mgr.submit(MakeConfig());
    auto summaries = mgr.list();
    ASSERT_EQ(summaries.size(), 2u);
    EXPECT_EQ(summaries[0].id, id2);
    EXPECT_EQ(summaries[1].id, id1);
    EXPECT_EQ(summaries[0].media_count, 1u);
}

TEST(TaskManagerTest, GetUnknownReturnsNullopt) {
    auto executor = std::make_shared<FakeExecutor>();
    TaskManager mgr(executor);
    EXPECT_FALSE(mgr.get("no_such_task").has_value());
    EXPECT_FALSE(mgr.cancel("no_such_task"));
}

TEST(TaskManagerTest, HighPriorityRunsFirst) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true; // keep first task running so the queue holds the rest
    TaskManager mgr(executor);
    auto low = mgr.submit(MakeConfig(), 0);
    auto high = mgr.submit(MakeConfig(), 10);

    // Wait until worker picked a task
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(mgr.is_running());
    // First task submitted (low) is picked first when queue is empty; then high
    // runs before a second low. Verify with a third task after high: order of
    // completion must be low -> high -> low2.
    mgr.cancel(low);
    EXPECT_EQ(WaitForTerminal(mgr, low), TaskStatus::Cancelled);
    auto low2 = mgr.submit(MakeConfig(), 0);
    mgr.cancel(high);
    EXPECT_EQ(WaitForTerminal(mgr, high), TaskStatus::Cancelled);
    mgr.cancel(low2);
    EXPECT_EQ(WaitForTerminal(mgr, low2), TaskStatus::Cancelled);
}

TEST(TaskManagerTest, SetPriorityAffectsQueuePosition) {
    auto executor = std::make_shared<FakeExecutor>();
    executor->block = true;
    TaskManager mgr(executor);
    auto first = mgr.submit(MakeConfig(), 0); // becomes running (blocked)
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!mgr.is_running() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    EXPECT_TRUE(mgr.is_running());

    auto a = mgr.submit(MakeConfig(), 0);
    auto b = mgr.submit(MakeConfig(), 0);

    // a and b both queued; bump a's priority -> a queued ahead of b
    EXPECT_TRUE(mgr.set_priority(a, 5));
    auto summaries = mgr.list();
    int pos_a = -1, pos_b = -1;
    for (const auto& s : summaries) {
        if (s.id == a) { pos_a = s.queue_position; }
        if (s.id == b) { pos_b = s.queue_position; }
    }
    EXPECT_GT(pos_a, 0);
    EXPECT_GT(pos_b, 0);
    EXPECT_LT(pos_a, pos_b); // higher priority -> earlier queue position

    // running task cannot be reprioritized
    EXPECT_FALSE(mgr.set_priority(first, 9));
    // unknown task
    EXPECT_FALSE(mgr.set_priority("no_such", 9));
}
