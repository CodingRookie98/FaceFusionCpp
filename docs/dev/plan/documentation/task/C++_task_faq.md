# {FAQ} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `design.md` 5.3.1, `C++_troubleshooting.md`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `faq.md` (中英双语)，解决常见报错和性能问题。

*   **目标**: 创建 `docs/user/en/faq.md` 和 `docs/user/zh/faq.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Standard

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/faq.md` | **New** | 英文版 FAQ |
| **User Doc (ZH)** | `docs/user/zh/faq.md` | **New** | 中文版 FAQ |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/user/en/faq.md` 不存在。
*   [ ] 检查 `docs/user/zh/faq.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Installation Issues (安装问题)**
    *   DLL missing.
    *   CUDA not found.
    *   Virus scan false positive.
2.  **Runtime Errors (运行时错误)**
    *   `E1xx` System Errors (Memory allocation failed).
    *   `E2xx` Config Errors (File not found, Invalid YAML).
    *   `E3xx` Model Errors (ONNX load failed, Shape mismatch).
    *   `E4xx` Runtime Errors (CUDA OOM, TensorRT build failed).
3.  **Performance Tuning (性能调优)**
    *   Why is the first run slow? (TensorRT engine building).
    *   How to speed up? (Enable caching, reduce quality, parallel execution).
4.  **Quality Issues (质量问题)**
    *   Face flickering.
    *   Colors mismatch.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 错误码与代码一致。
*   [ ] 解决方案可操作。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **错误码准确**: 对应 `foundation/infrastructure/error.ixx` (如果存在)。
*   [ ] **解决方案有效**: 经过验证的修复方法。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
