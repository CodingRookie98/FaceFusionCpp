<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的"物理布局"和"接口设计"。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
⚠️ TDD 强制：所有代码必须通过 TDD 流程产生 (🔴 Red → 🟢 Green → 🔵 Refactor)。
-->

# 添加 Autotools 依赖至 Linux CI 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: N/A (紧急修复)
> *   相关评估: N/A

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: N/A (紧急修复)
*   [x] **Target 检查**: 已确认修改目标为 `.github/actions/install-deps-linux/action.yml`。
*   [x] **冲突检查**: 文件已存在。
*   [x] **TDD 承诺**: 虽然是 YAML 修改，但我将通过手动验证和语法检查确保正确性。

## 1. 任务概览

### 1.1 目标
*   **目标**: 在 Linux 依赖安装 Action 中添加 `autoconf`, `autoconf-archive`, `automake`, `libtool`。
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CI Config** | `.github/actions/install-deps-linux/action.yml` | Modify | 添加 autotools 相关包 |

---

## 2. 实现流程

### 2.1 修改配置
在 `apt-get install` 命令中加入以下包：
- `autoconf`
- `autoconf-archive`
- `automake`
- `libtool`

### 2.2 验证
- 检查 YAML 语法。
- 确认包名在 Ubuntu 仓库中正确。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [x] **语法检查**: YAML 格式正确。
*   [x] **依赖确认**: 包名符合 Ubuntu 标准。

---
**执行人**: Antigravity (Sisyphus-Junior)
**开始日期**: 2026-02-16
**完成日期**: 2026-02-16
