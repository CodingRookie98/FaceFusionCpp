<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# PipelineRunner 集成任务单 (Phase 2)

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../../../C++_quality_standard.md)
> *   所属计划: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
> *   相关评估: [链接至对应评估](../../../evaluation/C++_evaluation_cli_phase2.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p3_cli/C++_plan_cli_implementation.md`。
*   [x] **依赖检查**: `services.pipeline.runner` 模块已准备就绪。
*   [x] **Config Parser**: `config.parser` 模块可用。

## 1. 任务概览

### 1.1 目标
> @brief 将 `PipelineRunner` 集成到 CLI 中。实现配置加载、Runner 实例化和任务执行。

*   **目标**: 运行 `FaceFusionCpp.exe -c task_config.yaml` 能够成功执行视频处理任务。
*   **所属计划**: [P3 CLI 实施计划](../C++_plan_cli_implementation.md)
*   **优先级**: High

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **CLI Implementation** | `src/app/cli/app_cli.cpp` | Modify | 引入 ConfigParser 和 PipelineRunner，实现 `run_task` 逻辑 |
| **CLI Header** | `src/app/cli/app_cli.ixx` | Modify | 导入必要的配置和 Service 模块 |

## 2. 详细设计与实现

### 2.1 依赖引入 (app_cli.ixx)

```cpp
// app_cli.ixx
import config.parser;
import services.pipeline.runner;
import foundation.infrastructure.logger;
// ... imports
```

### 2.2 执行逻辑 (app_cli.cpp)

```cpp
// app_cli.cpp

// Helper function
void run_pipeline(const std::string& config_path) {
    using namespace config;
    using namespace services::pipeline;

    // 1. Load Config
    auto config_result = ConfigParser::parse(config_path);
    if (config_result.is_err()) {
        std::cerr << "Config Error: " << config_result.error().message << std::endl;
        return;
    }
    auto task_config = config_result.value();

    // 2. Setup AppConfig (Default for now, later load from app_config.yaml)
    AppConfig app_config;
    app_config.models.path = "assets/models"; // Default

    // 3. Create Runner
    auto runner = CreatePipelineRunner(app_config);

    // 4. Run
    std::cout << "Starting task: " << task_config.task_info.id << std::endl;
    auto result = runner->Run(task_config, [](const TaskProgress& p) {
        // Simple progress feedback
        std::cout << "\rFrame: " << p.current_frame << "/" << p.total_frames 
                  << " (" << p.fps << " FPS)" << std::flush;
    });

    if (result.is_err()) {
        std::cerr << "\nPipeline failed: " << result.error().message << std::endl;
    } else {
        std::cout << "\nTask completed successfully." << std::endl;
    }
}
```

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: 确保模块链接正确。
*   [ ] **集成测试**: 创建一个临时的 `test_config.yaml`，运行 CLI 验证是否触发 Pipeline。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
