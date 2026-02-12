# {Getting Started Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有文档: [README.md](../../../../../README.md), [build.md](../../../../build.md)

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。
*   [x] **TDD 承诺**: 我确认将先创建空文件/占位符（Red），再填充内容（Green），最后检查格式（Refactor）。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `getting_started.md` (中英双语)，帮助用户完成环境准备、安装和首次运行。

*   **目标**: 创建 `docs/user/en/getting_started.md` 和 `docs/user/zh/getting_started.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Critical

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/getting_started.md` | **New** | 英文版快速入门指南 |
| **User Doc (ZH)** | `docs/user/zh/getting_started.md` | **New** | 中文版快速入门指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
> **目标**: 确认目标文件尚不存在。

*   [ ] 检查 `docs/user/en/getting_started.md` 不存在。
*   [ ] 检查 `docs/user/zh/getting_started.md` 不存在。

### 2.2 🟢 Green: 编写内容
> **目标**: 编写完整文档内容。

#### 2.2.1 内容大纲
1.  **System Requirements (系统要求)**
    *   Hardware: CUDA 12.2+, CUDNN 9.2+
    *   Software: FFmpeg 7.0.2+
2.  **Installation (安装)**
    *   Download from Release page.
    *   Extract zip.
3.  **First Run (首次运行)**
    *   Prepare source and target images.
    *   Command: `FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg`
    *   Explain `-s`, `-t`, `-o` briefly.
4.  **Verification (验证结果)**
    *   Check output file.

### 2.3 🔵 Refactor: 优化与检查
> **目标**: 确保双语一致性，格式正确。

*   [ ] 检查 Markdown 格式（标题、代码块）。
*   [ ] 确保中英文内容对应。
*   [ ] 检查链接是否有效。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **内容准确性**: 环境要求与 `README.md` 一致。
*   [ ] **命令可运行**: 示例命令符合 CLI 规范。
*   [ ] **双语一致**: 中文翻译准确。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
