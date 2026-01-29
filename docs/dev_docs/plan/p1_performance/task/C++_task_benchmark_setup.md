<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# 性能基准测试环境搭建任务单

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../C++_quality_standard.md)
> *   所属计划: [P1 性能优化计划](../C++_plan_performance_optimization.md)
> *   相关评估: [链接至对应评估](../evaluation/C++_evaluation_benchmark_setup.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p1_performance/C++_plan_performance_optimization.md` 并理解架构。
*   [x] **Target 检查**: 我已检查 `tests/integration/app` 并确认将创建新的测试源文件。
*   [x] **冲突检查**: 我已验证即将创建的文件名 `pipeline_benchmark_test.cpp` 不存在。

## 1. 任务概览

### 1.1 目标
> @brief 建立一个标准化的性能基准测试，用于测量 Pipeline 处理 100 帧视频的耗时和内存分配情况，作为后续优化的对比基准。

*   **目标**: 创建 `PipelineBenchmarkTest`，集成到测试套件中。
*   **所属计划**: [P1 性能优化计划](../C++_plan_performance_optimization.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)
> **规范**: 明确本任务涉及的具体模块文件。
> **约束**: 确保 `New` 文件不会覆盖现有工作，除非标记为 `Modify`。

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Test Implementation** | `tests/integration/app/pipeline_benchmark_test.cpp` | **New** | 实现 Benchmark 测试逻辑 |
| **Build Configuration** | `tests/integration/CMakeLists.txt` | Modify | 注册新的测试 Target |
| **Test Support** | `tests/test_support/performance_monitor.ixx` | **New** | (可选) 简单的性能计时器工具 |

---

## 2. 详细设计与实现

### 2.1 接口设计 (Benchmark Logic)

测试将模拟实际生产环境：
1.  加载标准测试视频。
2.  配置全流程 Pipeline (Swapper + Enhancer)。
3.  循环运行 N 次（例如处理 100 帧，或者重复处理同一视频）。
4.  记录总耗时、FPS。

### 2.2 实现逻辑 (Implementation / .cpp)

```cpp
// tests/integration/app/pipeline_benchmark_test.cpp
#include <gtest/gtest.h>
#include <chrono>
// ... imports

class PipelineBenchmarkTest : public ::testing::Test {
    // Setup resources
};

TEST_F(PipelineBenchmarkTest, Benchmark100Frames) {
    // 1. Configure Pipeline
    // 2. Start Timer
    auto start = std::chrono::high_resolution_clock::now();
    
    // 3. Run Pipeline
    runner->Run(config);
    
    // 4. Stop Timer
    auto end = std::chrono::high_resolution_clock::now();
    
    // 5. Report
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "[BENCHMARK] Total Time: " << duration << "ms" << std::endl;
    // ... calculate FPS
}
```

### 2.3 单元测试策略
此任务本身就是编写测试，验证标准为：
1.  测试能成功运行并输出数据。
2.  测试在合理时间内完成（不超时）。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: MSVC / Ninja 构建成功，无 Error。
*   [ ] **运行测试**: `python build.py --action test --test-regex "PipelineBenchmarkTest"` 成功输出性能数据。
*   [ ] **基准记录**: 将第一次运行的数据记录在 Findings 中。

---

## 4. 问题记录与错误追踪 (Issue Log & Error Protocol)

| #    | 问题描述 | 尝试次数 | 尝试方法 | 失败原因 | 解决方案   | 状态        |
| :--- | :------- | :------- | :------- | :------- | :--------- | :---------- |
| 1    |          |          |          |          |            | Open        |

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
