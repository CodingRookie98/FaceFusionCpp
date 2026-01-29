<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# CLI 基础框架搭建任务单 (Phase 1)

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../../../C++_quality_standard.md)
> *   所属计划: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
> *   相关评估: [链接至对应评估](../../../evaluation/C++_evaluation_cli_phase1.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p3_cli/C++_plan_cli_implementation.md` 并理解架构。
*   [x] **依赖检查**: 我已确认 `config.parser` 模块可用。
*   [x] **Target 检查**: 我将创建 `src/app/cli` 目录和相应的 CMake 目标。

## 1. 任务概览

### 1.1 目标
> @brief 搭建 `app.cli` 模块的基础骨架，实现 `App` 类，并更新 `main.cpp` 调用该模块。同时实现基础的参数解析 (`--help`, `--version`, `--config`)。

*   **目标**: 编译通过，运行 `FaceFusionCpp.exe --help` 输出信息。
*   **所属计划**: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CLI Implementation** | `src/app/cli/app_cli.ixx` | **New** | 定义 `app.cli` 模块接口 |
| **CLI Implementation** | `src/app/cli/app_cli.cpp` | **New** | 实现 CLI 逻辑 |
| **Build Config** | `src/app/cli/CMakeLists.txt` | **New** | 定义 `app_cli` 库 |
| **Entry Point** | `src/main.cpp` | Modify | 移除 Hello World，调用 `app::cli::App::run` |
| **Root Build** | `src/CMakeLists.txt` | Modify | 添加 `app/cli` 子目录 |

## 2. 详细设计与实现

### 2.1 接口设计 (app.cli)

```cpp
// src/app/cli/app_cli.ixx
export module app.cli;

import <string>;
import <vector>;
import <iostream>;

export namespace app::cli {

class App {
public:
    static int run(int argc, char** argv);

private:
    static void print_help();
    static void print_version();
};

}
```

### 2.2 实现逻辑 (app_cli.cpp)

简单的手动参数解析（避免引入额外依赖，保持轻量）：

```cpp
// src/app/cli/app_cli.cpp
module app.cli;

// imports...

namespace app::cli {

int App::run(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--help" || args[i] == "-h") {
            print_help();
            return 0;
        }
        if (args[i] == "--version" || args[i] == "-v") {
            print_version();
            return 0;
        }
        if (args[i] == "--config" || args[i] == "-c") {
            if (i + 1 < args.size()) {
                // TODO: Load config
                std::cout << "Loading config from: " << args[i+1] << std::endl;
                return 0;
            } else {
                std::cerr << "Error: --config requires a path argument" << std::endl;
                return 1;
            }
        }
    }
    
    print_help(); // Default action
    return 0;
}

// ... helpers
}
```

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: `python build.py --action build` 成功。
*   [ ] **Help 验证**: `build/msvc-x64-debug/bin/FaceFusionCpp.exe --help` 输出帮助。
*   [ ] **Version 验证**: `build/msvc-x64-debug/bin/FaceFusionCpp.exe --version` 输出版本。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
