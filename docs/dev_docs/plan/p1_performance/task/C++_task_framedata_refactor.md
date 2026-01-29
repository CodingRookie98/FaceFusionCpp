<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# FrameData 重构任务单

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../C++_quality_standard.md)
> *   所属计划: [P1 性能优化计划](../C++_plan_performance_optimization.md)
> *   相关评估: [链接至对应评估](../evaluation/C++_evaluation_framedata_refactor.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p1_performance/C++_plan_performance_optimization.md` 并理解架构。
*   [x] **依赖检查**: 我已确认 `src/services/pipeline/types.ixx` 是 `FrameData` 定义的位置。

## 1. 任务概览

### 1.1 目标
> @brief 重构 `FrameData` 结构，将高频元数据（如 Face Embeddings, Landmarks）提升为强类型成员，减少 `std::map<std::string, std::any>` 的使用，从而降低内存分配和查找开销。

*   **目标**: 优化 `FrameData` 结构，提升访问效率。
*   **所属计划**: [P1 性能优化计划](../C++_plan_performance_optimization.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Types Definition** | `src/services/pipeline/runner_types.ixx` (or `types.ixx` partition) | Modify | 添加强类型字段到 `FrameData` |
| **Adapter Implementation** | `src/domain/pipeline/pipeline_adapters.ixx` | Modify | 更新 Adapter 以使用新字段 |
| **Processor Implementation** | `src/services/pipeline/processors/face_analysis_processor.ixx` | Modify | 更新 Processor 以填充新字段 |
| **Runner Implementation** | `src/services/pipeline/runner_video.cpp` | Modify | 更新 Runner 填充 source embedding |

## 2. 详细设计与实现

### 2.1 结构定义 (FrameData)

```cpp
// src/services/pipeline/types.ixx (partition)

export struct FrameData {
    long long sequence_id = 0;
    bool is_end_of_stream = false;
    
    // Core Data
    cv::Mat image;

    // Optimized Metadata (Strongly Typed)
    // Avoid std::any for high-frequency data
    std::vector<float> source_embedding;               // From Runner
    std::vector<domain::face::Face> detected_faces;    // From Face Analysis
    
    // Processor Specific Inputs (Pre-calculated by Analysis Step)
    // Using std::optional to indicate presence without map lookup
    std::optional<domain::face::swapper::SwapInput> swap_input;
    std::optional<domain::face::enhancer::EnhanceInput> enhance_input;
    std::optional<domain::face::expression::RestoreExpressionInput> expression_input;
    
    // Legacy Metadata (for flexibility/extensibility only)
    std::map<std::string, std::any> metadata; 
};
```

### 2.2 迁移策略

1.  **FaceAnalysisProcessor**: 不再将结果打包进 `std::any` map，而是直接填充 `FrameData` 的 `swap_input`, `enhance_input` 等字段。
2.  **Adapters**: 直接访问 `frame.swap_input` 等字段，移除 `try_cast` 和 map 查找。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: 修改涉及多个核心文件，确保无编译错误。
*   [ ] **测试通过**: 运行 `PipelineBenchmarkTest`，确保功能未退化。
*   [ ] **性能对比**: 运行 Benchmark，观察 FPS 是否有提升（预期微小提升，但代码安全性大幅提高）。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
