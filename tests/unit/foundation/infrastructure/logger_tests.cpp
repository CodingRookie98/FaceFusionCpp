
/**
 * @file logger_tests.cpp
 * @brief Unit tests for Logger.
 * @author CodingRookie
 * @date
 * 2026-01-27
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <stdexcept>

import foundation.infrastructure.logger;

using namespace foundation::infrastructure::logger;

TEST(LoggerTest, SingletonInstance) {
    auto instance1 = Logger::get_instance();
    auto instance2 = Logger::get_instance();
    ASSERT_NE(instance1, nullptr);
    EXPECT_EQ(instance1, instance2);
}

TEST(LoggerTest, LogLevelsSmokeTest) {
    auto logger = Logger::get_instance();
    EXPECT_NO_THROW(logger->trace("Test trace"));
    EXPECT_NO_THROW(logger->debug("Test debug"));
    EXPECT_NO_THROW(logger->info("Test info"));
    EXPECT_NO_THROW(logger->warn("Test warn"));
    EXPECT_NO_THROW(logger->error("Test error"));
    EXPECT_NO_THROW(logger->critical("Test critical"));
}

TEST(LoggerTest, StaticLogMethod) {
    EXPECT_NO_THROW(Logger::log("info", "Test static log info"));
    EXPECT_NO_THROW(Logger::log("error", "Test static log error"));
}

TEST(LoggerTest, LogEnumMethod) {
    auto logger = Logger::get_instance();
    EXPECT_NO_THROW(logger->log(LogLevel::Info, "Test enum log info"));
}

TEST(LoggerTest, ParseSizeString) {
    EXPECT_EQ(parse_size_string("100"), 100);
    EXPECT_EQ(parse_size_string("1KB"), 1024);
    EXPECT_EQ(parse_size_string("1kb"), 1024);
    EXPECT_EQ(parse_size_string("1MB"), 1024 * 1024);
    EXPECT_EQ(parse_size_string("1GB"), 1024ULL * 1024 * 1024);

    // Test decimal values
    // 1.5 KB = 1.5 * 1024 = 1536
    EXPECT_EQ(parse_size_string("1.5KB"), 1536);

    EXPECT_THROW(parse_size_string(""), std::invalid_argument);
    EXPECT_THROW(parse_size_string("invalid"), std::invalid_argument);
    EXPECT_THROW(parse_size_string("KB"), std::invalid_argument);
}

TEST(LoggerTest, ConfigDefaults) {
    LoggingConfig config;
    EXPECT_EQ(config.level, LogLevel::Info);
    EXPECT_EQ(config.directory, "./logs");
    EXPECT_EQ(config.rotation, RotationPolicy::Daily);
    EXPECT_EQ(config.max_files, 7);
    EXPECT_EQ(config.max_total_size_bytes, 1ULL << 30);
}

TEST(LoggerTest, Initialization) {
    // Note: initialize can only be called effectively once due to std::call_once
    // We try to call it here with a test config.
    // If other tests ran before, it might be already initialized or skipped.
    // But since get_instance() is called in other tests, call_once in initialize might still work
    // if init_flag is local to initialize(). Yes, it is local static.

    LoggingConfig config;
    config.level = LogLevel::Debug;
    config.directory = "test_logs";
    config.rotation = RotationPolicy::Hourly;

    EXPECT_NO_THROW(Logger::initialize(config));

    if (Logger::is_initialized()) {
        auto logger = Logger::get_instance();
        EXPECT_NO_THROW(logger->debug("Debug message after init"));
    }
}
