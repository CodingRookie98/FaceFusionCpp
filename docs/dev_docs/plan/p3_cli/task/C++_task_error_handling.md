<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# 错误处理与健壮性任务单 (Phase 3)

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../../../C++_quality_standard.md)
> *   所属计划: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
> *   相关评估: [链接至对应评估](../../../evaluation/C++_evaluation_cli_phase3.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p3_cli/C++_plan_cli_implementation.md`。
*   [x] **依赖检查**: `app.cli` 模块已就绪。

## 1. 任务概览

### 1.1 目标
> @brief 增强 CLI 的错误处理能力。捕获并优雅报告各类异常（配置错误、运行时错误、系统异常），确保程序不会崩溃退出，而是输出有意义的错误信息。

*   **目标**: `try-catch` 覆盖主循环，错误信息输出到 stderr。
*   **所属计划**: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CLI Implementation** | `src/app/cli/app_cli.cpp` | Modify | 添加全局 try-catch 块，优化错误输出 |

## 2. 详细设计与实现

### 2.1 全局异常捕获 (app_cli.cpp)

```cpp
int App::run(int argc, char** argv) {
    try {
        // ... existing logic ...
    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Fatal Error: Unknown exception occurred." << std::endl;
        return 1;
    }
    return 0;
}
```

### 2.2 错误报告优化

当 Pipeline 返回错误时，如果是预期的错误（如文件不存在），输出简洁信息。如果是未预期的（Result::Err），输出详细原因。

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **异常测试**: 手动抛出一个异常，验证是否被捕获并打印。
*   [ ] **配置错误**: 传入错误的配置文件路径，验证是否输出 "Config Error"。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
