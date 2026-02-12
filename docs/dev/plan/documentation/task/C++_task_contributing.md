# {Contributing Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `AGENTS.md`, `workflow.md`, `build.md`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `contributing.md` (中英双语)，规范贡献流程。

*   **目标**: 创建 `docs/dev/en/contributing.md` 和 `docs/dev/zh/contributing.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P2-High

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Dev Doc (EN)** | `docs/dev/en/contributing.md` | **New** | 英文版贡献指南 |
| **Dev Doc (ZH)** | `docs/dev/zh/contributing.md` | **New** | 中文版贡献指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/dev/en/contributing.md` 不存在。
*   [ ] 检查 `docs/dev/zh/contributing.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Code of Conduct (行为准则)**
    *   Respectful, inclusive.
2.  **Getting Started (入门)**
    *   Fork & Clone.
    *   Setup Environment (Link to Build Guide).
3.  **Development Workflow (开发流程)**
    *   **Branching**: `feature/xyz` or `fix/issue-123`. No direct push to main.
    *   **TDD**: Write test first (Red), Implement (Green), Refactor.
    *   **Style**: clang-format, clang-tidy.
4.  **Pull Request Process (PR 流程)**
    *   Pass all tests locally (`python build.py --action test`).
    *   Clean commit history (squash if needed).
    *   Description template.
5.  **Commit Messages (提交信息规范)**
    *   Conventional Commits (`feat:`, `fix:`, `docs:`, `refactor:`).

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 确保引用了 TDD 强制要求。
*   [ ] 格式化清晰。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **流程闭环**: 从分支到合并的完整路径。
*   [ ] **强制性**: 明确禁止无测试提交。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
