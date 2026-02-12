# {CLI Reference} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `src/application/app_cli.ixx`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `cli_reference.md` (中英双语)，详细说明命令行参数用法。

*   **目标**: 创建 `docs/user/en/cli_reference.md` 和 `docs/user/zh/cli_reference.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Critical

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/cli_reference.md` | **New** | 英文版 CLI 参考 |
| **User Doc (ZH)** | `docs/user/zh/cli_reference.md` | **New** | 中文版 CLI 参考 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/user/en/cli_reference.md` 不存在。
*   [ ] 检查 `docs/user/zh/cli_reference.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Usage Syntax**
    *   `FaceFusionCpp [options]`
2.  **Options Table (Full List)**
    *   `-c, --config <file>`: Load configuration from file.
    *   `-s, --source <path>`: Source image/video path.
    *   `-t, --target <path>`: Target image/video path.
    *   `-o, --output <path>`: Output directory.
    *   `--processors <list>`: List of processors (face_swapper, face_enhancer, etc.).
    *   `--system-check`: Perform system check and exit.
    *   `--validate`: Validate configuration and exit.
    *   `--log-level <level>`: Set log level (trace, debug, info, warn, error).
3.  **Examples**
    *   Basic Face Swap.
    *   Face Enhancement only.
    *   Using Config File.
    *   System Check.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 格式化表格。
*   [ ] 确保参数名称与代码一致。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **参数完整性**: 涵盖所有支持的参数。
*   [ ] **示例准确性**: 示例命令可直接运行。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
