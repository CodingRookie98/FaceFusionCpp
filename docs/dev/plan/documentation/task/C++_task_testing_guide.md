# {Testing Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `tests/README.md`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `testing_guide.md` (中英双语)，说明测试框架和编写规范。

*   **目标**: 创建 `docs/dev/en/testing_guide.md` 和 `docs/dev/zh/testing_guide.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P2-Standard

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Dev Doc (EN)** | `docs/dev/en/testing_guide.md` | **New** | 英文版测试指南 |
| **Dev Doc (ZH)** | `docs/dev/zh/testing_guide.md` | **New** | 中文版测试指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/dev/en/testing_guide.md` 不存在。
*   [ ] 检查 `docs/dev/zh/testing_guide.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Test Categories (测试分类)**
    *   **Unit**: `tests/unit/`. Isolated, fast.
    *   **Integration**: `tests/integration/`. Components interaction.
    *   **E2E**: `tests/e2e/`. Full pipeline.
2.  **Running Tests (运行测试)**
    *   `python build.py --action test`
    *   `python build.py --action test --test-label unit`
3.  **Writing Tests (编写测试)**
    *   GTest / GMock.
    *   Fixture usage.
    *   Output path helper (`tests/common/test_paths.h`).

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 确保命令准确。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **分类准确**: 符合项目现状。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
