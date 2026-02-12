# {Architecture Design} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `design.md`, `src/` (扫描模块依赖)

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `architecture.md` (中英双语)，详细介绍系统架构和关键设计决策。

*   **目标**: 创建 `docs/dev/en/architecture.md` 和 `docs/dev/zh/architecture.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P2-High

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Dev Doc (EN)** | `docs/dev/en/architecture.md` | **New** | 英文版架构文档 |
| **Dev Doc (ZH)** | `docs/dev/zh/architecture.md` | **New** | 中文版架构文档 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/dev/en/architecture.md` 不存在。
*   [ ] 检查 `docs/dev/zh/architecture.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Overview (概览)**
    *   **5-Layer Architecture**: Application -> Services -> Domain -> Platform -> Foundation.
    *   **Dependency Rule**: Unidirectional (Upper depends on Lower).
    *   **Modular C++20**: Interface (.ixx) / Implementation (.cpp) separation.
2.  **Layers Detail (层级详解)**
    *   **Application**: CLI, Config Parser.
    *   **Services**: Pipeline Runner, Shutdown Handler.
    *   **Domain**: Business Logic (Face, Image, Video), Model Registry.
    *   **Platform**: Hardware Abstraction (File System, OS).
    *   **Foundation**: Core Utils, Logger, AI Inference Engine (ORT/TensorRT).
3.  **Key Design Decisions (关键设计决策)**
    *   **PIMPL**: Hide implementation details, speed up compilation.
    *   **Factory Pattern**: Create processor instances dynamically.
    *   **RAII**: Resource management (InferenceSession, CUDA Memory).
4.  **Diagrams (架构图)**
    *   Mermaid diagram showing the 5 layers and key modules.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 确保 Mermaid 图渲染正常。
*   [ ] 术语与 `design.md` 一致。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **层级准确**: 5层架构描述正确。
*   [ ] **代码对应**: 提到的模块在 `src/` 中确实存在。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
