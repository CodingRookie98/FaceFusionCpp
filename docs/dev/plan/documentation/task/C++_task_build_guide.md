# {Build Guide} 实施任务单

> **标准参考 & 跨文档链接**:
> *   所属计划: [项目文档输出实施计划](../C++_plan_documentation.md)
> *   现有代码: `build.py`, `CMakePresets.json`, `docs/build.md`

## 0. 任务前验证 (AI Agent 自检)

*   [x] **父计划**: 我已阅读 `docs/dev/plan/documentation/C++_plan_documentation.md`。
*   [x] **冲突检查**: 我已验证即将创建的文件名不存在。

## 1. 任务概览

### 1.1 目标
> @brief 编写 `build_guide.md` (中英双语)，帮助开发者搭建环境并构建项目。

*   **目标**: 创建 `docs/dev/en/build_guide.md` 和 `docs/dev/zh/build_guide.md`。
*   **所属计划**: [项目文档输出实施计划](../C++_plan_documentation.md)
*   **优先级**: P2-High

### 1.2 模块变更清单

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Dev Doc (EN)** | `docs/dev/en/build_guide.md` | **New** | 英文版构建指南 |
| **Dev Doc (ZH)** | `docs/dev/zh/build_guide.md` | **New** | 中文版构建指南 |

---

## 2. TDD 实现流程

### 2.1 🔴 Red: 验证文件缺失
*   [ ] 检查 `docs/dev/en/build_guide.md` 不存在。
*   [ ] 检查 `docs/dev/zh/build_guide.md` 不存在。

### 2.2 🟢 Green: 编写内容
#### 2.2.1 内容大纲
1.  **Prerequisites (前置条件)**
    *   **OS**: Windows 10/11 x64, Linux (Ubuntu 22.04+).
    *   **Compiler**: MSVC (VS 2022 v17.10+), GCC 13+ (Linux).
    *   **Tools**: CMake 3.28+, Python 3.10+, Ninja.
    *   **Libraries**: CUDA 12.2+, TensorRT 10.x.
2.  **Environment Setup (环境搭建)**
    *   Install Visual Studio / Build Tools.
    *   Install CUDA & TensorRT.
    *   Install Python deps (`requirements.txt` if any, or just `colorama`).
3.  **Building with build.py (使用脚本构建)**
    *   `python build.py --action configure`
    *   `python build.py --action build`
    *   `python build.py --action test`
    *   `python build.py --action install`
4.  **CMake Presets (预设详解)**
    *   `msvc-x64-debug` / `msvc-x64-release`
    *   `linux-x64-debug` / `linux-x64-release`
5.  **Troubleshooting (构建问题)**
    *   Common CMake errors.
    *   vcpkg issues.

### 2.3 🔵 Refactor: 优化与检查
*   [ ] 确保命令准确。
*   [ ] 检查链接有效性。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)
*   [ ] **版本要求**: 与 `build.py` 和 `CMakeLists.txt` 一致。
*   [ ] **步骤完整**: 包含从克隆代码到运行的所有步骤。

---
**执行人**: Antigravity
**开始日期**: 2026-02-12
