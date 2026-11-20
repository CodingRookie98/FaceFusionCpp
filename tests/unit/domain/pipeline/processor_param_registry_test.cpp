#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>

import processor.param_registry;

using namespace domain::processor;

// Helper to find a param by name in ProcessorMeta
const ParamMeta* find_param(const ProcessorMeta* meta, const std::string& name) {
    if (!meta) return nullptr;
    for (const auto& p : meta->params) {
        if (p.name == name) return &p;
    }
    return nullptr;
}

class ProcessorParamRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Singleton persists; tests use unique names to avoid collision
    }
};

TEST_F(ProcessorParamRegistryTest, RegisterAndFindProcessor) {
    std::string name = "test_proc_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor(
        {.name = name,
         .params = {
             {"model", ParamType::String, {"model_a", "model_b"}, "Model name"},
             {"factor", ParamType::Float, {}, "Blend factor", std::make_pair(0.0, 1.0)},
         }});

    auto* meta = ProcessorParamRegistry::instance().find(name);
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->name, name);
    EXPECT_EQ(meta->params.size(), 2u);
    EXPECT_EQ(meta->params[0].name, "model");
    EXPECT_EQ(meta->params[0].type, ParamType::String);
    EXPECT_EQ(meta->params[0].allowed_values, (std::vector<std::string>{"model_a", "model_b"}));
    EXPECT_EQ(meta->params[1].name, "factor");
    EXPECT_EQ(meta->params[1].type, ParamType::Float);
    EXPECT_TRUE(meta->params[1].range.has_value());
    EXPECT_DOUBLE_EQ(meta->params[1].range->first, 0.0);
    EXPECT_DOUBLE_EQ(meta->params[1].range->second, 1.0);
}

TEST_F(ProcessorParamRegistryTest, FindUnknownReturnsNull) {
    auto* meta = ProcessorParamRegistry::instance().find("nonexistent_proc_xyz_98765");
    EXPECT_EQ(meta, nullptr);
}

TEST_F(ProcessorParamRegistryTest, AllProcessorNamesIncludesRegistered) {
    std::string name = "test_list_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor({.name = name, .params = {}});

    auto names = ProcessorParamRegistry::instance().all_processor_names();
    EXPECT_NE(std::find(names.begin(), names.end(), name), names.end());
}

TEST_F(ProcessorParamRegistryTest, IsValidProcessor) {
    std::string name = "test_valid_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor({.name = name, .params = {}});

    EXPECT_TRUE(ProcessorParamRegistry::instance().is_valid_processor(name));
    EXPECT_FALSE(ProcessorParamRegistry::instance().is_valid_processor("no_such_proc_abc_12345"));
}

TEST_F(ProcessorParamRegistryTest, ParamTypeVariants) {
    std::string name = "test_types_" + std::to_string(rand());
    ProcessorParamRegistry::instance().register_processor(
        {.name = name,
         .params = {
             {"str_param", ParamType::String, {"hello", "world"}, "A string"},
             {"int_param", ParamType::Int, {}, "An int", std::make_pair(0.0, 100.0)},
             {"float_param", ParamType::Float, {}, "A float", std::make_pair(0.0, 1.0)},
             {"bool_param", ParamType::Bool, {}, "A bool"},
             {"path_param", ParamType::Path, {}, "A path"},
         }});

    auto* meta = ProcessorParamRegistry::instance().find(name);
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->params.size(), 5u);
    EXPECT_EQ(meta->params[0].type, ParamType::String);
    EXPECT_EQ(meta->params[1].type, ParamType::Int);
    EXPECT_EQ(meta->params[2].type, ParamType::Float);
    EXPECT_EQ(meta->params[3].type, ParamType::Bool);
    EXPECT_EQ(meta->params[4].type, ParamType::Path);
}

TEST_F(ProcessorParamRegistryTest, RegistrarHelperRegisters) {
    std::string name = "test_registrar_" + std::to_string(rand());
    {
        ProcessorParamRegistrar registrar(
            {.name = name, .params = {{"x", ParamType::Int, {}, "An int"}}});
    }
    // registrar goes out of scope, but registration should persist
    auto* meta = ProcessorParamRegistry::instance().find(name);
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->params.size(), 1u);
    EXPECT_EQ(meta->params[0].name, "x");
}
