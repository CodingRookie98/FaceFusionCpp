<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# 日志重构与 CLI11 集成任务单 (Refactor)

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../../../C++_quality_standard.md)
> *   所属计划: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
> *   相关评估: [链接至对应评估](../../../evaluation/C++_evaluation_cli_refactor.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p3_cli/C++_plan_cli_implementation.md`。
*   [x] **依赖检查**: `CLI11` 和 `spdlog` (via `foundation.infrastructure.logger`) 已可用。

## 1. 任务概览

### 1.1 目标
> @brief 将 CLI 模块中的手动参数解析替换为 `CLI11`，并将 `std::cout/cerr` 替换为结构化日志。

*   **目标**: 使用 CLI11 解析参数，日志输出规范化。
*   **所属计划**: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CLI Implementation** | `src/app/cli/app_cli.cpp` | Modify | 引入 CLI11 和 Logger |
| **Build Config** | `src/app/cli/CMakeLists.txt` | Modify | 链接 CLI11 |

## 2. 详细设计与实现

### 2.1 日志替换 (Logger)

```cpp
import foundation.infrastructure.logger;
using foundation::infrastructure::logger::Logger;

Logger::get_instance()->info("Starting task: {}", task_config.task_info.id);
Logger::get_instance()->error("Config Error: {}", config_result.error().message);
```

### 2.2 CLI11 集成

```cpp
#include <CLI/CLI.hpp>

int App::run(int argc, char** argv) {
    CLI::App app{"FaceFusionCpp"};
    argv = app.ensure_utf8(argv);

    std::string config_path;
    bool show_version = false;

    app.add_option("-c,--config", config_path, "Path to task configuration file");
    app.add_flag("-v,--version", show_version, "Show version information");

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    if (show_version) {
        print_version();
        return 0;
    }

    if (!config_path.empty()) {
        run_pipeline(config_path);
        return 0;
    }
    
    // Show help if no action taken
    std::cout << app.help() << std::endl;
    return 0;
}
```

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: 确保 CLI11 头文件找到。
*   [ ] **功能验证**: `-h`, `-v`, `-c` 功能正常。
*   [ ] **日志验证**: 控制台看到带时间戳的日志输出。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-30
