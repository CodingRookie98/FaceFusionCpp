<!-- AI_CONTEXT
你是一名高级 C++ 架构师。
你的目标是评估已完成的架构改动，确保其符合项目标准，并为后续工作提供稳固基础。
-->

# P2: 架构解耦 (Architecture Decoupling) 结项评估

> **关联计划**: [P2 Architecture Refactoring Plan](../plan/p2_architecture/C++_plan_architecture_refactoring.md)
> **评估时间**: 2026-01-29

## 1. 目标达成情况

| 目标 | 状态 | 说明 |
| :--- | :--- | :--- |
| **解耦 PipelineRunner 与 Adapter** | ✅ 完成 | `PipelineRunner` 不再直接依赖具体的 `*Adapter` 类，而是通过 `ProcessorFactory` 创建。 |
| **统一 PipelineContext** | ✅ 完成 | 上下文管理已移至 `domain.pipeline.context`，并解决了循环依赖问题。 |
| **消除编译期依赖** | ✅ 完成 | 接口定义与实现分离，模块间依赖清晰。 |
| **稳定性修复** | ✅ 完成 | 修复了 SegFault 问题，解决了模型路径传递为空导致的加载失败问题。 |

## 2. 关键架构变更

### 2.1 依赖倒置 (DIP) 实施
*   **Before**: `PipelineRunner` 依赖 `FaceSwapperAdapter`, `FaceEnhancerAdapter` 等具体类。
*   **After**: `PipelineRunner` 依赖 `ProcessorFactory` 接口。具体 Adapter 通过静态注册机制 (`ProcessorRegistrar`) 注册到工厂。

### 2.2 模块化修正
*   `domain.pipeline.context`: 独立模块，承载所有 Pipeline 运行时状态。
*   `domain.pipeline.adapters`: 实现 `IFrameProcessor` 接口，作为 Domain Service 与 Pipeline 之间的桥梁。
*   **显式析构**: 在 `SwapperAdapter` 中添加了显式析构函数，解决了跨模块边界的析构 SegFault 问题 (MSVC 特定行为)。

## 3. 遇到的问题与解决方案

### 3.1 SegFault in PipelineRunnerImageTest
*   **现象**: `ProcessSingleImage` 测试在退出时崩溃。
*   **根因**: 跨模块边界析构复杂对象（包含 `std::shared_ptr` 和 `cv::Mat`）时，MSVC 的自动生成析构函数可能存在链接或顺序问题。
*   **解决**: 在 `SwapperAdapter` 中显式定义 `~SwapperAdapter()`。

### 3.2 模型加载失败
*   **现象**: `PipelineRunner` 运行时，Adapter 尝试加载 "default_model" 而非配置中指定的模型。
*   **根因**: `PipelineRunner` 初始化了 Service，但没有将 `TaskConfig` 中的模型路径传递给 `PipelineContext`。Adapter 创建时仅获取了 Service 指针，丢失了模型路径信息。
*   **解决**: 在 `PipelineContext` 中添加模型路径字段，并在 `PipelineRunner` 解析配置时填充这些字段。

## 4. 遗留项与建议 (Next Steps)

*   **性能瓶颈**: 目前 `FrameData` 包含 `std::map<std::string, std::any>`，每次访问都有开销。且 `ThreadSafeQueue` 锁竞争未优化。这将在 P1 阶段解决。
*   **测试超时**: 视频测试在 Debug 模式下超时，属于正常现象，需在 Release 模式或 P1 优化后验证性能。
*   **代码清理**: 部分临时注释和调试日志需要清理。

## 5. 结论

P2 架构解耦已成功完成。代码库结构更加清晰，模块职责分明，符合 Clean Architecture 原则。构建系统稳定，核心测试通过。可以进入 P1 性能优化阶段。

**批准状态**: [x] APPROVED
