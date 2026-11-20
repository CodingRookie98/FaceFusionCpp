/**
 * @file inference_options_test.cpp
 * @brief 推理 Provider 配置映射行为测试（BL-2）
 * @details 验证 config.inference_options 的公开契约：名称解析、优先级裁决、
 *          可用性过滤、闭包展开与 Options 映射。
 */

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

import config.app;
import config.inference_options;

using namespace config;
using foundation::ai::inference_session::ExecutionProvider;
using foundation::ai::inference_session::Options;
using ProviderSet = std::unordered_set<ExecutionProvider>;

// ============================================================================
// parse_execution_provider
// ============================================================================

TEST(ParseExecutionProviderTest, AcceptsDocumentedNamesCaseInsensitively) {
    EXPECT_EQ(parse_execution_provider("tensorrt"), ExecutionProvider::TensorRT);
    EXPECT_EQ(parse_execution_provider("TensorRT"), ExecutionProvider::TensorRT);
    EXPECT_EQ(parse_execution_provider("TENSORRT"), ExecutionProvider::TensorRT);
    EXPECT_EQ(parse_execution_provider("cuda"), ExecutionProvider::CUDA);
    EXPECT_EQ(parse_execution_provider("CUDA"), ExecutionProvider::CUDA);
    EXPECT_EQ(parse_execution_provider("cpu"), ExecutionProvider::CPU);
    EXPECT_EQ(parse_execution_provider("Cpu"), ExecutionProvider::CPU);
}

TEST(ParseExecutionProviderTest, RejectsUnknownNames) {
    EXPECT_EQ(parse_execution_provider("rocm"), std::nullopt);
    EXPECT_EQ(parse_execution_provider("directml"), std::nullopt);
    EXPECT_EQ(parse_execution_provider(""), std::nullopt);
}

// ============================================================================
// resolve_execution_providers
// ============================================================================

TEST(ResolveExecutionProvidersTest, ExplicitCpuWinsOverAvailableGpu) {
    const ProviderSet resolved = resolve_execution_providers(
        {"cpu"}, {ExecutionProvider::TensorRT, ExecutionProvider::CUDA, ExecutionProvider::CPU});

    EXPECT_EQ(resolved, ProviderSet{ExecutionProvider::CPU});
}

TEST(ResolveExecutionProvidersTest, SkipsUnavailableHighPriorityProvider) {
    const ProviderSet resolved = resolve_execution_providers(
        {"tensorrt", "cuda", "cpu"}, {ExecutionProvider::CUDA, ExecutionProvider::CPU});

    EXPECT_EQ(resolved, (ProviderSet{ExecutionProvider::CUDA, ExecutionProvider::CPU}));
}

TEST(ResolveExecutionProvidersTest, ExpandsTensorRtClosure) {
    const ProviderSet resolved = resolve_execution_providers(
        {"tensorrt"},
        {ExecutionProvider::TensorRT, ExecutionProvider::CUDA, ExecutionProvider::CPU});

    EXPECT_EQ(resolved, (ProviderSet{ExecutionProvider::TensorRT, ExecutionProvider::CUDA,
                                     ExecutionProvider::CPU}));
}

TEST(ResolveExecutionProvidersTest, EmptyConfigFallsBackToAuto) {
    const ProviderSet resolved =
        resolve_execution_providers({}, {ExecutionProvider::CUDA, ExecutionProvider::CPU});

    EXPECT_EQ(resolved, (ProviderSet{ExecutionProvider::CUDA, ExecutionProvider::CPU}));
}

TEST(ResolveExecutionProvidersTest, AllUnknownFallsBackToAuto) {
    const ProviderSet resolved =
        resolve_execution_providers({"rocm", "directml"}, {ExecutionProvider::CPU});

    EXPECT_EQ(resolved, ProviderSet{ExecutionProvider::CPU});
}

TEST(ResolveExecutionProvidersTest, NeverReturnsEmptySetWhenProviderUnavailable) {
    // CPU 恒可用，即便可用性集合异常为空也必须给出可运行的兜底。
    const ProviderSet resolved = resolve_execution_providers({"tensorrt"}, {});

    EXPECT_FALSE(resolved.empty());
    EXPECT_EQ(resolved, ProviderSet{ExecutionProvider::CPU});
}

// ============================================================================
// make_inference_options
// ============================================================================

TEST(MakeInferenceOptionsTest, MapsProviderPriorityAndDevice) {
    InferenceConfig config;
    config.default_providers = {"cpu"};
    config.device_id = 3;

    const Options options = make_inference_options(config);

    EXPECT_EQ(options.execution_providers, ProviderSet{ExecutionProvider::CPU});
    EXPECT_EQ(options.execution_device_id, 3);
}

TEST(MakeInferenceOptionsTest, MapsTensorRtEngineCacheSettings) {
    InferenceConfig config;
    config.engine_cache.enable = false;
    config.engine_cache.path = "./custom/cache";

    const Options options = make_inference_options(config);

    EXPECT_FALSE(options.enable_tensorrt_cache);
    EXPECT_EQ(options.engine_cache_path, "./custom/cache");
}
