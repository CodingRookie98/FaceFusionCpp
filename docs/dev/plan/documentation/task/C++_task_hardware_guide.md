# {Hardware Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `design.md` A.3.3, `design_roadmap.md` 10.4

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `hardware_guide.md` (中英双语)，详细介绍硬件需求和优化策略。

*   **目标**: 创建 `docs/user/en/hardware_guide.md` 和 `docs/user/zh/hardware_guide.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Standard

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/hardware_guide.md` | **New** | 英文版硬件指南 |
| **User Doc (ZH)** | `docs/user/zh/hardware_guide.md` | **New** | 中文版硬件指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/user/en/hardware_guide.md` 不存在。
*   [ ] 检查 `docs/user/zh/hardware_guide.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **GPU Tiers (GPU 分级)**
    *   **Flagship** (>= 12GB VRAM): 4090, 3090, 4080. Parallel + High Quality.
    *   **Mainstream** (8-12GB VRAM): 4070, 3080. Standard usage.
    *   **Entry** (4-8GB VRAM): 3060, 4060. Strict memory mode.
    *   **Low-End** (< 4GB): Not recommended, but possible with limitations.
2.  **Memory Optimization (显存优化)**
    *   `memory_strategy: strict`
    *   `execution_order: sequential`
    *   `max_queue_size: 1`
3.  **Performance Expectations (性能期望)**
    *   Approximate FPS for different resolutions (720p, 1080p, 4K) on different cards.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 数据合理性检查 (基于 benchmarks).
*   [ ] 确保中英文对应。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **分级合理**: 符合当前硬件市场情况。
*   [ ] **优化有效**: 提供的优化策略确实能降低显存。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
