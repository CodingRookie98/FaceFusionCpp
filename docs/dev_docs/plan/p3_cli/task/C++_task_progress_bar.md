<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# 进度条与优雅退出任务单 (Phase 2 & 3)

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
> @brief 完善 CLI 体验，添加控制台进度条显示，并实现 Ctrl+C 信号捕获以优雅退出 Pipeline。

*   **目标**: 进度条实时更新，按下 Ctrl+C 后程序能清理资源并退出。
*   **所属计划**: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CLI Implementation** | `src/app/cli/app_cli.cpp` | Modify | 实现 `indicators` 进度条和信号处理 |
| **Build Config** | `src/app/cli/CMakeLists.txt` | Modify | 链接 `indicators` 库 (如果可用) 或手写简单进度条 |

## 2. 详细设计与实现

### 2.1 信号处理 (Signal Handling)

为了跨平台支持，使用 `std::signal` 或 Windows API (`SetConsoleCtrlHandler`)。

```cpp
// app_cli.cpp
#include <csignal>
#include <atomic>

std::atomic<bool> g_interrupted{false};

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "\nInterrupt received. Stopping pipeline...\n";
        g_interrupted = true;
        // Trigger PipelineRunner::Cancel() if instance is global/accessible
    }
}
```

**改进**: `App` 类需要持有 Runner 实例，并提供静态访问点或通过 Lambda 捕获。

### 2.2 进度条实现 (Progress Bar)

使用 `indicators` 库 (vcpkg 已安装) 或简单的文本输出。

```cpp
#include <indicators/progress_bar.hpp>

// Inside callback
static indicators::ProgressBar bar{
    indicators::option::BarWidth{50},
    indicators::option::Start{"["},
    indicators::option::Fill{"="},
    indicators::option::Lead{">"},
    indicators::option::Remainder{" "},
    indicators::option::End{"]"},
    indicators::option::ShowElapsedTime{true},
    indicators::option::ShowRemainingTime{true},
    indicators::option::FontStyles{std::vector<indicators::FontStyle>{indicators::FontStyle::bold}}
};

float progress = (float)p.current_frame / p.total_frames * 100.0f;
bar.set_progress(progress);
```

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **进度显示**: 运行任务时看到进度条动画。
*   [ ] **中断测试**: 运行长任务，中途按 Ctrl+C，验证程序是否在几秒内退出。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
