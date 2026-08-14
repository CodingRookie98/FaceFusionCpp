# 分层架构文档评估报告 (Layered Architecture Doc Assessment)

本报告对 [layers.md](../architecture/layers.md)（5层架构实现细节，V1.0.0，中英文双语）及 [design.md](../architecture/design.md)（V3.1）§1.2 分层描述进行完整性与实现一致性评估。评估方式为逐节对照当前代码基线（`src/` 目录树、各层 `CMakeLists.txt` 依赖方向、代码符号 grep），识别架构文档与实际代码之间的偏差。**本报告处置建议已按 P0-P3 顺序实施完毕（layers.md 中英文 V1.0.0 → V2.0.0 重构；design.md 中文 V3.1 → V3.2、英文 V2.9 → V2.10）。**

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-EVAL-LAYERS-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已实施 (Implemented)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-14 | AI Agent | 王辉 | 处置建议已按 P0-P3 全部实施：layers.md 中英文重构为 4 层结构（V2.0.0），design.md 中英分层引用同步修正（zh V3.2 / en V2.10）；本报告状态更新为已实施。 |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 初版：基于代码基线完成 layers.md 与 design.md §1.2 分层一致性评估。 |

---

## 1. 评估概述

- **评估对象**: `docs/dev/zh/architecture/layers.md`、`docs/dev/en/architecture/layers.md`（内容与中文版一致）、`docs/dev/zh/architecture/design.md` §1.2（5层分层架构图）
- **评估基线**:
  - `src/` 目录树（`find src -maxdepth 1`）
  - 各层 `CMakeLists.txt` 的 `target_link_libraries` 依赖方向
  - 代码符号检索（`FaceModelRegistry`、`file_system`、`thread_pool`、FFmpeg 封装位置等）
- **评估维度**: 目录结构真实性、组件归属正确性、依赖规则落地性、文档完备性（层↔目录↔CMake target 映射）

---

## 2. 与实现一致（✅ 无需改动）

| 章节 | 结论 |
|---|---|
| §2.1 Application 层 | ✅ `src/app/` 存在：AppConfig (`app_config.ixx`)、TaskConfig (`task_config.ixx`)、CLI (`app_cli.ixx`) 全部匹配 |
| §2.2 Services 层 | ✅ `src/services/pipeline/` 存在：PipelineRunner (`pipeline_runner.ixx`)、ShutdownHandler (`shutdown_handler.ixx`)、CheckpointManager (`checkpoint_manager.ixx`) 全部匹配 |
| §2.3 Domain 层 FaceModelRegistry | ✅ `src/domain/face/analyser/face_model_registry.cpp` 存在（含 get_instance/set_instance_for_testing） |
| §2.5 Foundation 层 InferenceEngine/Logger/Utilities | ✅ `src/foundation/ai/inference_session.ixx`、`infrastructure/logger.ixx`、`infrastructure/`（信号量/并发队列/线程池）存在 |
| §3 依赖规则 | ✅ 依赖方向实测单向：`app→{services,domain,foundation}`、`services→{domain,foundation}`、`domain→foundation`（无反向链接） |
| design.md §1.2 依赖单向性原则 | ✅ 与代码实际一致 |

---

## 3. 文档与实现不一致（❌ 需修正）

| # | 章节 | 文档内容 | 代码实际 | 严重度 |
|---|---|---|---|---|
| 1 | layers.md §2.4 | **Layer 4: Platform (`src/platform/`)**，含 FileSystem、Threading | **`src/platform/` 目录不存在**。`src/` 仅 4 个顶层目录：`app`/`services`/`domain`/`foundation`（`src/CMakeLists.txt` 仅 4 行 add_subdirectory）。FileSystem (`file_system.ixx`、`concurrent_file_system.ixx`) 与 Threading (`thread_pool.ixx`) 实际位于 `src/foundation/infrastructure/` | 🔴 高（文档描述了不存在的目录结构，5 层实为 4 层） |
| 2 | layers.md §2.3 | "Image/Video Wrappers: 针对 OpenCV 和 FFmpeg 的业务逻辑封装" 位于 Domain | FFmpeg 封装实际位于 **`src/foundation/media/`**（`ffmpeg.ixx`、`ffmpeg_reader.cpp`、`ffmpeg_writer.cpp`、`vision.cpp`）；`src/domain/` 下无 media 封装，仅有 face/frame/pipeline/ai/common 子域 | 🟡 中（组件归属错误） |
| 3 | layers.md 全文 | 仅 54 行，无层↔目录↔CMake target 映射、无每层内部模块清单 | 代码实际有 17 个 CMake target（app_version/app_cli/domain_face/domain_frame/domain_pipeline/foundation_ai/foundation_infrastructure/foundation_media/services_pipeline 等）与大量子模块，文档完全未覆盖 | 🟡 中（作为"实现细节"文档信息密度不足，无法指导新人定位代码） |
| 4 | design.md §1.2 | 5 层架构图含 Platform 层（App→Svc→Dom→Plat→Fdn） | 与 #1 同源：实际无 Platform 层 | 🟡 中（与 layers.md 同源错误，需同步修正） |

---

## 4. 处置建议

| 优先级 | 处置 | 涉及项 |
|---|---|---|
| P0 | 重构 layers.md：删除虚构的 Platform 层，改为准确的 **4 层结构**（Application→Services→Domain→Foundation）；FileSystem/Threading 归入 Foundation 内部 infrastructure 分组说明 | #1 |
| P1 | 修正组件归属：Image/Video Wrappers（FFmpeg/OpenCV 封装）明确标注位于 `src/foundation/media/`；Domain 层补充真实子域（face/frame/pipeline/ai/common）清单 | #2 |
| P1 | 补全层↔目录↔CMake target 映射表与每层关键模块清单（从实际代码提取），使文档可作为代码定位地图 | #3 |
| P2 | 同步修正 design.md §1.2 分层图（4 层 + Foundation 内部子分组说明），保持两文档一致 | #4 |
| P2 | 中英文版 layers.md 同步更新 | #1-#4 |

**推荐方案**：文档迁就代码事实（4 层），不新建 `src/platform/` 目录——foundation/infrastructure 已完整容纳 fs/线程职责，强行拆分增加维护成本且无架构收益。

---

## 5. 附录：实际代码结构参考（评估基线）

```
src/                          # 4 层（非 5 层）
├── app/                      # Layer 1: Application（CLI、配置入口）
│   ├── cli/                  #   app_cli.ixx, system_check.ixx
│   └── config/               #   app_config.ixx, task_config.ixx, config_types.ixx,
│                             #   config_validator.ixx, config_merger.ixx, parser/
├── services/                 # Layer 2: Services（业务编排）
│   └── pipeline/             #   pipeline_runner.ixx, runner_image.cpp, runner_video.cpp,
│                             #   checkpoint_manager.ixx, shutdown_handler.ixx, metrics_collector.ixx
├── domain/                   # Layer 3: Domain（AI 核心与图像编排）
│   ├── ai/                   #   model_repository.ixx
│   ├── common/               #   common.ixx, common_types.ixx
│   ├── face/                 #   detector/ landmarker/ recognizer/ swapper/ enhancer/
│   │                         #   masker/ classifier/ analyser/ (face_model_registry)
│   ├── frame/                #   enhancer/ (frame_enhancer.ixx, impl/)
│   └── pipeline/             #   pipeline_api.ixx, pipeline_adapters.ixx, queue.ixx,
│                             #   processor_factory.ixx, processor_param_registry.ixx
└── foundation/               # Layer 4: Foundation（底座）
    ├── ai/                   #   inference_session.ixx, session_pool.ixx, inference_session_registry.ixx
    ├── infrastructure/       #   file_system.ixx, thread_pool.ixx, concurrent_queue.ixx,
    │                         #   logger.ixx, console.ixx, progress.ixx, network.ixx, process.ixx
    └── media/                #   ffmpeg.ixx, ffmpeg_reader.cpp, ffmpeg_writer.cpp, vision.cpp
```

**依赖方向实测**（`target_link_libraries` 汇总）：
- `app → {foundation, domain, services}`（含 app_cli 直链 CUDA/cuDNN/TensorRT 用于 system-check 探测）
- `services_pipeline → {domain, foundation}`
- `domain_* → foundation_*`
- 无任何反向/跨层循环依赖 ✅
