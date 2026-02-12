# {User Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `design.md` 4.1, `design_roadmap.md` M6

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `user_guide.md` (中英双语)，详细介绍核心功能和工作流。

*   **目标**: 创建 `docs/user/en/user_guide.md` 和 `docs/user/zh/user_guide.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Standard

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/user_guide.md` | **New** | 英文版用户指南 |
| **User Doc (ZH)** | `docs/user/zh/user_guide.md` | **New** | 中文版用户指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/user/en/user_guide.md` 不存在。
*   [ ] 检查 `docs/user/zh/user_guide.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Core Features (核心功能)**
    *   Face Swapping: How it works, multi-face vs single face.
    *   Face Enhancement: Detail restoration (GFPGAN).
    *   Frame Enhancement: Super-resolution (Real-ESRGAN).
2.  **Workflows (工作流)**
    *   **Image Processing**: Source(s) -> Target Image -> Output.
    *   **Video Processing**: Source(s) -> Target Video -> Output.
    *   **Batch Processing**: Using config files for multiple tasks.
3.  **Processor Combination (处理器组合)**
    *   Why combine? (Swap -> Enhance).
    *   Order matters (Swap first, then Enhance).

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 检查术语统一性 (Face Swapper vs FaceSwapper).
*   [ ] 检查双语对应。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **功能覆盖**: 涵盖所有已实现功能。
*   [ ] **逻辑清晰**: 工作流步骤易懂。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
