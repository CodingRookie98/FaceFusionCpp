
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>

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
