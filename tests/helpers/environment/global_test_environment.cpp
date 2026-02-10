// tests/integration/common/global_test_environment.cpp
#include <gtest/gtest.h>
#include <cstdlib>
#include <iostream>

#ifdef CUDA_SYNC_AVAILABLE
#include <cuda_runtime.h>
#endif

import domain.face.model_registry;
import foundation.ai.inference_session_registry;

// 全局清理环境，防止 CUDA 驱动关闭后的析构崩溃
// 该类会在所有测试运行结束后调用 TearDown
class GlobalCleanupEnvironment : public ::testing::Environment {
public:
    void TearDown() override {
        std::cout << "[GlobalCleanupEnvironment] Starting TearDown..." << std::endl;

        // 显式清理单例资源，确保在 main 返回前释放 CUDA 资源
        // 必须遵循 "依赖者先释放" 原则，否则可能导致 Heap Corruption
        //
        // 依赖链: FaceModelRegistry -> FaceModel -> InferenceSession <- SessionRegistry (cache)

        // 1. 先清理 FaceModelRegistry (释放 FaceModel 对 InferenceSession 的引用)
        // ModelRegistry 持有 FaceModel，FaceModel 持有 InferenceSession 的 shared_ptr
        try {
            std::cout << "[GlobalCleanupEnvironment] Clearing FaceModelRegistry..." << std::endl;
            domain::face::FaceModelRegistry::get_instance()->clear();
        } catch (...) {
            std::cerr << "[GlobalCleanupEnvironment] Exception clearing FaceModelRegistry"
                      << std::endl;
        }

        // 2. 再清理 SessionRegistry (此时 session 引用计数已降低，仅剩 cache 引用)
        // SessionRegistry 持有 InferenceSession 的 shared_ptr cache
        try {
            std::cout << "[GlobalCleanupEnvironment] Clearing InferenceSessionRegistry..."
                      << std::endl;
            foundation::ai::inference_session::InferenceSessionRegistry::get_instance()->clear();
        } catch (...) {
            std::cerr << "[GlobalCleanupEnvironment] Exception clearing InferenceSessionRegistry"
                      << std::endl;
        }

        // 3. 等待 CUDA 操作完成
#ifdef CUDA_SYNC_AVAILABLE
        std::cout << "[GlobalCleanupEnvironment] Synchronizing CUDA device..." << std::endl;
        cudaError_t err = cudaDeviceSynchronize();
        if (err != cudaSuccess) {
            std::cerr << "[GlobalCleanupEnvironment] CUDA sync failed: " << cudaGetErrorString(err)
                      << std::endl;
        }
#endif

        // 4. 强制退出，跳过静态对象析构
        // TensorRT Myelin 异步回调在静态析构阶段会访问已失效的 CUDA 上下文
        // 使用 _exit() 直接终止进程，避免触发这些回调
        const char* force_exit = std::getenv("TEARDOWN_FORCE_EXIT");
        if (!force_exit || std::string(force_exit) != "0") {
            std::cout
                << "[GlobalCleanupEnvironment] Forcing exit via _exit(0) to skip static destruction."
                << std::endl;
            _exit(0); // 跳过 atexit handlers 和静态析构
        } else {
            std::cout
                << "[GlobalCleanupEnvironment] TEARDOWN_FORCE_EXIT=0, proceeding to normal exit (expect crashes)."
                << std::endl;
        }
    }
};

// 注册全局环境
// 只要此文件被链接到测试二进制中，就会自动注册该环境
namespace {
::testing::Environment* const cleanup_env =
    ::testing::AddGlobalTestEnvironment(new GlobalCleanupEnvironment);
}

// 强制链接符号
void LinkGlobalTestEnvironment() {}
