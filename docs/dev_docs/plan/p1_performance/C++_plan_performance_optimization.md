<!-- AI_CONTEXT
你是一名高级 C++ 架构师。
你的目标是设计一个可扩展且严格遵循项目工程指南的 C++20 模块系统。
在填写此模板时，你必须根据项目的“元规则”验证每一个决定。
-->

# P1: 性能优化 (Performance Optimization) 实施计划

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../C++_quality_standard.md)
> *   相关任务单: 见各阶段的 "对应 Task" 链接
> *   评估报告: [链接至对应评估](../evaluation/C++_evaluation_p1_performance_optimization.md) (计划完成后创建)

## 0. 计划前验证 (AI Agent 自检)
> **指令**: 在编写本计划其余部分之前，列出你已阅读和分析的具体文件，以确保本计划基于事实。

*   [x] 我已阅读 `docs/C++/C++_20_modules_practice_guide.md`。
*   [x] 我已阅读 `src/services/pipeline/types.ixx` (FrameData 结构)。
*   [x] 我已阅读 `src/foundation/containers/thread_safe_queue.ixx` (队列实现)。
*   [x] 我已确认建议的优化策略符合 C++20 规范和项目约束。

**已检查的上下文:**
*   `src/services/pipeline/types.ixx`: `FrameData` 当前包含 `cv::Mat image`，`std::map<std::string, std::any> metadata`，拷贝成本较高。
*   `src/services/pipeline/pipeline_runner.cpp`: 数据在步骤间传递时可能存在拷贝。
*   `src/domain/pipeline/pipeline_adapters.cpp`: 适配器调用时的数据流向。

## 1. 计划概述

### 1.1 目标与范围
> @brief 解决 Pipeline 处理过程中的不必要数据拷贝和锁竞争问题，提升视频处理吞吐量。

*   **核心目标**: 
    1.  实现 `FrameData` 的 **零拷贝 (Zero-Copy)** 传递，使用移动语义 (Move Semantics)。
    2.  优化 `ThreadSafeQueue` 减少锁竞争，评估无锁队列的可行性。
    3.  确保视频处理 FPS 提升 20% 以上 (基准测试对比)。
*   **涉及模块**: 
    *   `services.pipeline.types`
    *   `services.pipeline.runner`
    *   `foundation.containers` (ThreadSafeQueue)
    *   `domain.pipeline` (Processor Interfaces)

### 1.2 关键约束 (C++20 & Modules)
> @brief 确认本项目特定的技术约束，确保符合大型工程规范

*   [x] **标准**: C++20 (MSVC/Clang/GCC 兼容)
*   [x] **构建**: CMake + Ninja (支持并行构建)
*   [x] **模块化**: 保持模块边界清晰，不破坏现有的 P2 解耦架构。
*   [x] **依赖**: 禁止反向依赖。
*   [x] **验证**: 我已阅读 `docs/C++/C++_20_modules_practice_guide.md` 并将在设计中应用。

---

## 2. 架构设计 (核心)

### 2.1 数据流优化策略 (Zero-Copy)

目前 `FrameData` 在 Pipeline 各阶段传递时，若不小心会触发 `cv::Mat` 的引用计数增加（轻量拷贝）或深拷贝。更严重的是 `std::map<std::string, std::any>` 的拷贝开销极大。

**优化方案**:
1.  **Move Semantics**: 强制所有 Processor 接口接收 `FrameData&&` 或 `std::unique_ptr<FrameData>`。
    *   *Decision*: 使用 `FrameData&` (引用) 进行原地修改，避免传递所有权，因为 Pipeline 是线性的。或者如果涉及多线程分发，使用 `std::shared_ptr<FrameData>` 传递。
    *   *Correction*: 当前 Pipeline 是顺序执行的，Processor 接口为 `process(FrameData& frame)`，这已经是引用的形式，理论上是 Zero-Copy。
    *   *Investigation Needed*: 检查 `PipelineRunner` 内部是否有多余的临时对象创建。
2.  **Metadata Optimization**: `std::any` 存储小对象有优化，但 `std::map` 每次查找都有开销。
    *   *Action*: 评估是否将常用元数据（如 `landmarks`）提升为 `FrameData` 的强类型成员，而非存放在 map 中。

### 2.2 队列与并发设计

`ThreadSafeQueue` 当前使用 `std::mutex` + `std::condition_variable`。

**优化方案**:
1.  **细粒度锁**: 确保锁仅保护队列操作，不包含业务逻辑。
2.  **批量操作**: 增加 `pop_batch` 接口，一次取多帧，减少锁争用频率。

### 2.3 依赖关系图 (逻辑视图)

```mermaid
graph TD
    PipelineRunner --> ThreadSafeQueue
    ThreadSafeQueue --> FrameData
    PipelineRunner --> ProcessorFactory
    ProcessorFactory --> IFrameProcessor
    IFrameProcessor ..-> FrameData : Modifies (In-Place)
```

---

## 3. 实施路线图

### 3.1 阶段一: 性能基准建立与 `FrameData` 优化
**目标**: 建立当前的性能基准 (Profiling)，并优化 `FrameData` 结构。

*   [ ] **任务 1.1**: 创建 Benchmark 测试用例 (处理 100 帧视频)，记录耗时与内存分配次数。 → 对应 Task: [plan/p1_performance/task/C++_task_benchmark_setup.md](./plan/p1_performance/task/C++_task_benchmark_setup.md)
*   [ ] **任务 1.2**: 审查 `FrameData` 结构，将高频元数据（Landmarks, Embeddings）提升为结构体成员，移除 `std::any` 的频繁装箱/拆箱。 → 对应 Task: [plan/p1_performance/task/C++_task_framedata_refactor.md](./plan/p1_performance/task/C++_task_framedata_refactor.md)
*   [ ] **验收标准**:
    *   Benchmark 脚本可运行并输出报告。
    *   `FrameData` 重构后编译通过，现有测试通过。

**阶段内研究检查点**:
| 时间 | 发现内容 | 来源 | 影响评估 |
| :--- | :--- | :--- | :--- |
| TBD | 当前 FPS 数据 | Profiling Tool | 设定具体优化目标 |

### 3.2 阶段二: 队列与并发模型优化
**目标**: 减少多线程环境下的锁竞争。

*   [ ] **任务 2.1**: 为 `ThreadSafeQueue` 实现 `try_pop` 和 `wait_and_pop_batch` 接口。 → 对应 Task: [plan/p1_performance/task/C++_task_queue_optimization.md](./plan/p1_performance/task/C++_task_queue_optimization.md)
*   [ ] **任务 2.2**: 更新 `PipelineRunner` 中的 `WorkerThread`，使用批量取帧模式（例如一次取 4 帧处理）。 → 对应 Task: [plan/p1_performance/task/C++_task_runner_batching.md](./plan/p1_performance/task/C++_task_runner_batching.md)
*   [ ] **验收标准**:
    *   并发测试无死锁。
    *   CPU 利用率提升。

### 3.3 阶段三: OpenCV 与 ONNX Runtime 调优
**目标**: 优化底层库的参数配置。

*   [ ] **任务 3.1**: 调整 OpenCV 线程池设置 (`cv::setNumThreads`)，避免与 Pipeline 线程池冲突。
*   [ ] **任务 3.2**: 优化 ONNX Runtime `SessionOptions` (Intra/Inter op threads) 配置。
*   [ ] **验收标准**:
    *   最终 Benchmark 显示性能提升 > 20%。

---

## 4. 风险管理

| 风险点 | 可能性 | 影响 | 缓解措施 |
| :--- | :--- | :--- | :--- |
| **重构引入 Bug** | 中 | 破坏现有功能 | 保持 `PipelineRunnerImageTest` 始终通过。 |
| **优化不明显** | 中 | 浪费开发时间 | 阶段一必须先做 Profiling，找到真正的瓶颈（可能是模型推理而非数据拷贝）。 |
| **内存泄漏** | 低 | 长期运行崩溃 | 使用 ASan (AddressSanitizer) 运行测试。 |

## 5. 资源与依赖
*   **工具**: MSVC Profiler / Intel VTune (可选)
*   **依赖**: OpenCV, ONNX Runtime
