#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>

import foundation.infrastructure.console;
import foundation.infrastructure.logger;
import foundation.infrastructure.progress;
import tests.common.fixtures.unit_test_fixture;

using namespace foundation::infrastructure::console;
using namespace foundation::infrastructure::logger;
using namespace foundation::infrastructure::progress;
using namespace tests::common::fixtures;
using namespace testing;

namespace {

class MockProgressController : public IProgressController {
public:
    MOCK_METHOD(void, suspend, (), (override));
    MOCK_METHOD(void, resume, (), (override));
};

class ConsoleIntegrationTest : public UnitTestFixture {};

TEST_F(ConsoleIntegrationTest, LoggerShouldSuspendActiveProgressBar) {
    MockProgressController mock_controller;

    ConsoleManager::instance().register_progress_bar(&mock_controller);

    {
        InSequence s;
        EXPECT_CALL(mock_controller, suspend()).Times(1);
        EXPECT_CALL(mock_controller, resume()).Times(1);
    }

    Logger::get_instance()->info("test message for console integration");

    ConsoleManager::instance().unregister_progress_bar(&mock_controller);
}

TEST_F(ConsoleIntegrationTest, ProgressBarShouldRegisterWithConsoleManager) {
    {
        ProgressBar bar("Test Bar");
        EXPECT_EQ(ConsoleManager::instance().active_controller(), &bar);
    }
    EXPECT_EQ(ConsoleManager::instance().active_controller(), nullptr);
}

} // namespace
