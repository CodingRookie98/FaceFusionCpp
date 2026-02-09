// tests/integration/common/global_test_environment.cpp
#include <gtest/gtest.h>

import domain.face.model_registry;
import foundation.ai.inference_session_registry;

// 全局清理环境，防止 CUDA 驱动关闭后的析构崩溃
// 该类会在所有测试运行结束后调用 TearDown
class GlobalCleanupEnvironment : public ::testing::Environment {
public:
    void TearDown() override {
        // 显式清理单例资源，确保在 main 返回前释放 CUDA 资源
        // 必须遵循 "依赖者先释放" 原则，否则可能导致 Heap Corruption
        //
        // 依赖链: FaceModelRegistry -> FaceModel -> InferenceSession <- SessionRegistry (cache)

        // 1. 先清理 FaceModelRegistry (释放 FaceModel 对 InferenceSession 的引用)
        // ModelRegistry 持有 FaceModel，FaceModel 持有 InferenceSession 的 shared_ptr
        domain::face::FaceModelRegistry::get_instance().clear();

        // 2. 再清理 SessionRegistry (此时 session 引用计数已降低，仅剩 cache 引用)
        // SessionRegistry 持有 InferenceSession 的 shared_ptr cache
        foundation::ai::inference_session::InferenceSessionRegistry::get_instance().clear();
    }
};

// 注册全局环境
// 只要此文件被链接到测试二进制中，就会自动注册该环境
namespace {
::testing::Environment* const cleanup_env =
    ::testing::AddGlobalTestEnvironment(new GlobalCleanupEnvironment);
}
