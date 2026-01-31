#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>

import services.pipeline.shutdown;

using namespace services::pipeline;

class ShutdownHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure clean state
        ShutdownHandler::uninstall();
    }

    void TearDown() override { ShutdownHandler::uninstall(); }
};

TEST_F(ShutdownHandlerTest, InitialStateIsRunning) {
    EXPECT_EQ(ShutdownHandler::get_state(), ShutdownState::Running);
    EXPECT_FALSE(ShutdownHandler::is_shutdown_requested());
}

TEST_F(ShutdownHandlerTest, InstallAndUninstall) {
    bool callback_called = false;
    ShutdownHandler::install([&]() { callback_called = true; });

    EXPECT_EQ(ShutdownHandler::get_state(), ShutdownState::Running);

    ShutdownHandler::uninstall();
    // Should be able to reinstall
    ShutdownHandler::install([&]() { callback_called = true; });
    ShutdownHandler::uninstall();
}

TEST_F(ShutdownHandlerTest, RequestShutdownTriggersCallback) {
    std::atomic<bool> callback_called{false};

    ShutdownHandler::install(
        [&]() {
            callback_called = true;
            ShutdownHandler::mark_completed();
        },
        std::chrono::seconds{2});

    ShutdownHandler::request_shutdown();

    // Wait for callback
    std::this_thread::sleep_for(std::chrono::milliseconds{100});

    EXPECT_TRUE(callback_called);
    EXPECT_TRUE(ShutdownHandler::is_shutdown_requested());
}

TEST_F(ShutdownHandlerTest, TimeoutTriggersTimeoutCallback) {
    std::atomic<bool> shutdown_called{false};
    std::atomic<bool> timeout_called{false};

    ShutdownHandler::install(
        [&]() {
            shutdown_called = true;
            // Simulate long-running cleanup (don't call mark_completed)
            std::this_thread::sleep_for(std::chrono::seconds{3});
        },
        std::chrono::seconds{1}, // Short timeout
        [&]() { timeout_called = true; });

    ShutdownHandler::request_shutdown();

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});

    EXPECT_TRUE(shutdown_called);
    EXPECT_TRUE(timeout_called);
    EXPECT_EQ(ShutdownHandler::get_state(), ShutdownState::TimedOut);
}

TEST_F(ShutdownHandlerTest, GracefulCompletionBeforeTimeout) {
    std::atomic<bool> completed{false};

    ShutdownHandler::install(
        [&]() {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
            completed = true;
            ShutdownHandler::mark_completed();
        },
        std::chrono::seconds{5});

    ShutdownHandler::request_shutdown();
    bool result = ShutdownHandler::wait_for_shutdown();

    EXPECT_TRUE(result);
    EXPECT_TRUE(completed);
    EXPECT_EQ(ShutdownHandler::get_state(), ShutdownState::Completed);
}
