#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>

import processor_factory;
import domain.pipeline;

using namespace domain::pipeline;
using namespace testing;

class MockFrameProcessor : public IFrameProcessor {
public:
    MOCK_METHOD(void, process, (FrameData&), (override));
    MOCK_METHOD(void, ensure_loaded, (), (override));
};

class ProcessorFactoryTest : public Test {
protected:
    void SetUp() override {
        // Factory is a singleton, so state persists.
        // We can register new processors, but we can't easily clear existing ones
        // without adding a clear method to the class.
        // For testing purposes, we use unique names.
    }
};

TEST_F(ProcessorFactoryTest, RegisterAndCreateProcessor) {
    std::string type = "MockType_" + std::to_string(rand());

    bool creator_called = false;
    ProcessorFactory::instance().register_processor(type, [&](const void* context) {
        creator_called = true;
        return std::make_shared<NiceMock<MockFrameProcessor>>();
    });

    auto processor = ProcessorFactory::instance().create(type, nullptr);

    EXPECT_TRUE(creator_called);
    EXPECT_NE(processor, nullptr);
}

TEST_F(ProcessorFactoryTest, CreateUnknownProcessorReturnsNull) {
    std::string type = "UnknownType_" + std::to_string(rand());

    auto processor = ProcessorFactory::instance().create(type, nullptr);

    EXPECT_EQ(processor, nullptr);
}

TEST_F(ProcessorFactoryTest, ContextIsPassedToCreator) {
    std::string type = "ContextType_" + std::to_string(rand());
    int dummy_context = 42;

    ProcessorFactory::instance().register_processor(type, [&](const void* context) {
        const int* val = static_cast<const int*>(context);
        EXPECT_NE(val, nullptr);
        EXPECT_EQ(*val, 42);
        return std::make_shared<NiceMock<MockFrameProcessor>>();
    });

    ProcessorFactory::instance().create(type, &dummy_context);
}

TEST_F(ProcessorFactoryTest, RegistrarAutomaticallyRegisters) {
    std::string type = "RegistrarType_" + std::to_string(rand());

    // Create a registrar instance, which should register the processor in constructor
    {
        ProcessorRegistrar registrar(
            type, [](const void*) { return std::make_shared<NiceMock<MockFrameProcessor>>(); });
    }

    auto processor = ProcessorFactory::instance().create(type, nullptr);
    EXPECT_NE(processor, nullptr);
}
