/**
 ******************************************************************************
 * @file           : process_tests.cpp
 * @brief          : Unit tests for process module
 ******************************************************************************
 */

#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>

import foundation.infrastructure.process;

using namespace foundation::infrastructure::process;

TEST(ProcessTest, BasicExecutionAndOutput) {
    std::string output;
    auto read_stdout = [&](const char* bytes, size_t n) { output.append(bytes, n); };

#ifdef _WIN32
    std::string command = "cmd /c echo hello world";
#else
    std::string command = "echo -n 'hello world'";
#endif

    Process process(command, "", read_stdout, nullptr, false);
    int exit_status = process.get_exit_status();

    EXPECT_EQ(exit_status, 0);

    // Normalize output (remove potential newlines if echo adds them despite -n or on windows)
    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }
    EXPECT_EQ(output, "hello world");
}

TEST(ProcessTest, ExitStatusError) {
#ifdef _WIN32
    std::string command = "cmd /c exit 1";
#else
    std::string command = "sh -c 'exit 1'";
#endif

    Process process(command);
    int exit_status = process.get_exit_status();

    EXPECT_EQ(exit_status, 1);
}
