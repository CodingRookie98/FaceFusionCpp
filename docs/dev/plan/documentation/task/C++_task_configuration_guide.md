# {Configuration Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `src/application/app_config.ixx`, `src/domain/task/task_config.ixx`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `configuration_guide.md` (中英双语)，详细说明配置文件字段与使用场景。

*   **目标**: 创建 `docs/user/en/configuration_guide.md` 和 `docs/user/zh/configuration_guide.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P1-Critical

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **User Doc (EN)** | `docs/user/en/configuration_guide.md` | **New** | 英文版配置指南 |
| **User Doc (ZH)** | `docs/user/zh/configuration_guide.md` | **New** | 中文版配置指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/user/en/configuration_guide.md` 不存在。
*   [ ] 检查 `docs/user/zh/configuration_guide.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Overview**
    *   Explain YAML format.
    *   Two main sections: `application_config` and `task_config`.
2.  **Application Config**
    *   `log_level`: trace, debug, info, warn, error.
    *   `system_resources`: gpu_memory_limit, cpu_threads.
    *   `paths`: output_dir, models_dir.
3.  **Task Config**
    *   `source_path`, `target_path`.
    *   `processors`: list of enabled processors.
    *   `face_swapper`: model, parameters.
    *   `face_enhancer`: model, blend_ratio.
4.  **Scenario Examples**
    *   **Basic Face Swap**: Only swap enabled.
    *   **High Quality**: Swap + Enhance + Restore.
    *   **Low Memory**: Limit GPU memory, sequential execution.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 确保 YAML 格式正确。
*   [ ] 字段名称与代码完全匹配。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **字段完整性**: 涵盖所有主要配置项。
*   [ ] **示例可用性**: 复制示例到文件可直接运行。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
