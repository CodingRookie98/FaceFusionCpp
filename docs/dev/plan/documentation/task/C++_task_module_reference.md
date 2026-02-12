# {Module Reference} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `src/**/*.ixx`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `module_reference.md` (中英双语)，列出关键模块及其职责。

*   **目标**: 创建 `docs/dev/en/module_reference.md` 和 `docs/dev/zh/module_reference.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P2-Standard

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Dev Doc (EN)** | `docs/dev/en/module_reference.md` | **New** | 英文版模块参考 |
| **Dev Doc (ZH)** | `docs/dev/zh/module_reference.md` | **New** | 中文版模块参考 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/dev/en/module_reference.md` 不存在。
*   [ ] 检查 `docs/dev/zh/module_reference.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲 (按层级分组)
1.  **Application**
    *   `app.cli`: CLI entry.
    *   `config`: Parsers.
2.  **Services**
    *   `services.pipeline`: Runner.
    *   `services.shutdown`: Signal handling.
3.  **Domain**
    *   `domain.face`: Face models.
    *   `domain.image`: Image IO.
4.  **Foundation**
    *   `foundation.ai`: ORT/TRT wrappers.
    *   `foundation.infrastructure`: Logger, Errors.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 检查模块名称是否准确。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **覆盖率**: 包含核心模块。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
