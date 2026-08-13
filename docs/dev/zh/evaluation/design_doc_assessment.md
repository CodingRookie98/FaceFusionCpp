# 功能设计文档评估报告 (Design Document Assessment)

本报告对 [design.md](../architecture/design.md)（应用层架构设计说明书，V2.9）进行完整性与实现一致性评估。评估方式为逐节对照当前代码基线（`src/` 源码、`config/` 实际配置、`vcpkg.json` 依赖清单），识别设计文档与实现之间的偏差，为后续修订提供决策依据。

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-EVAL-DESIGN-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 待审批 (Pending Review)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 初版：基于代码基线完成 design.md 全量一致性评估。 |

---

## 1. 评估概述

- **评估对象**: `docs/dev/zh/architecture/design.md`（V2.9，1009 行）
- **评估基线**:
  - `src/app/config/app_config.ixx`、`task_config.ixx`、`config_types.ixx`
  - `src/app/cli/app_cli.cpp`、`system_check.cpp`
  - `src/domain/pipeline/pipeline_adapters.cpp`、`processor_param_registry.ixx`
  - `src/services/pipeline/metrics_collector.ixx`、`checkpoint_manager.ixx`、`runner_video.cpp`
  - `config/app_config.yaml`、`config/task_config.yaml`、`vcpkg.json`
- **评估维度**: 设计完整性（文档是否覆盖全部实现）、实现一致性（文档描述是否与代码相符）、过时标注（已实现仍标"待定/规划"）、设计缺陷（配置/行为歧义）

---

## 2. 与实现一致（✅ 无需改动）

| 章节 | 结论 |
|---|---|
| §2.1 配置分离、§2.2 运行模式 | ✅ CLI 已实现，Server 标注 Future 正确 |
| §3.1 AppConfig schema | ✅ 字段全覆盖（inference/resource/logging/metrics/models/temp） |
| §3.2 任务配置 schema 主体 | ✅ TaskInfo/IO/Resource/FaceAnalysis/Pipeline 全部匹配 |
| §3.3 配置校验 | ✅ config_validator.ixx 已实现，E201/E202/E203 错误码一致，含 YAML path 定位 |
| §3.5.2-3.5.3 CLI 结构与参数 | ✅ CLI11 实现，`-s/-t/-o/--processors/--validate/--system-check/--json` 全匹配 |
| §3.5.4 system-check 输出 | ✅ 人类可读 + JSON 双格式，检查项（CUDA/cuDNN/VRAM/FFmpeg/ONNX/model_repo）匹配 |
| §4.1 处理器与适配器 | ✅ 4 个处理器（swapper/enhancer/restorer/frame_enhancer）注册与参数元数据全部匹配 |
| §4.2.1 Sequential/Batch 策略 | ✅ execution_order 枚举实现 |
| §5.6 优雅停机 | ✅ shutdown_handler 独立实现 |
| §5.7 并发与流控 | ✅ 有界队列 + semaphore 背压实现 |
| §5.11 Metrics JSON Schema | ✅ schema_version/task_id/duration_ms/summary/step_latency(p50,p99)/gpu_memory 全匹配 |
| §A.1-A.5 术语/素材/依赖 | ✅ 测试素材与硬件基准均与实测一致（slideshow_scaled.mp4 491帧 720×1280 确认无误） |

---

## 3. 设计已实现但文档仍标"待定/规划"（⚠️ 过时标注）

| 位置 | 文档现状 | 代码实际 | 建议 |
|---|---|---|---|
| §5.8 数据序列化 | 标注 ***Implementation Pending*** | **已实现**：`flatbuffers` 已在 vcpkg.json，`src/domain/face/schema/face_generated.h` 生成代码存在并使用 | 改为"已实现" |
| §5.3.1 错误码 | 标注 ***Planned***，"尚未完全实装" | **已实现**：config_types.ixx 完整 ErrorCode 枚举（E100-E206+，含 E104/E204/E205/E206 等文档未列的新码） | 改为"已实现"，补充新码 |
| §6.1 配置校验 | "部分实现" | 已完整实现（含 CLI --validate） | 改为"已实现" |

---

## 4. 文档与实现实质性不一致（❌ 需修正）

| # | 章节 | 文档内容 | 代码实际 | 严重度 |
|---|---|---|---|---|
| 1 | §3.5.3 CLI 参数表 | `-c, --config` 载入任务配置 | 实际为 **`-c, --task-config`**（app_cli.cpp:140） | 🔴 高（用户按文档操作会失败） |
| 2 | §3.5.3 CLI 参数表 | 处理器 flag 表格静态列了 4 组 | 已由 ProcessorParamRegistry **动态生成**（文档 §3.5.1 已提到元数据驱动，但表格给人静态印象；`--face-swapper-model` 等 kebab-case 生成规则与文档一致） | 🟡 中（表述需澄清为示例） |
| 3 | §3.1 AppConfig schema | 无 `default_models` 段、无 `metrics.gpu_sample_interval_ms` | 实际 app_config.yaml **有 `default_models`（6 个默认模型）+ `gpu_sample_interval_ms: 1000`** | 🟡 中（文档缺字段） |
| 4 | §3.2 TaskConfig schema | `resource` 无 `memory_strategy`、无 `max_frames` | TaskResourceConfig 实际有 `memory_strategy`（默认 Tolerant）与 `max_frames`；且 TaskConfig 级 memory_strategy **与 AppConfig 级默认 Strict 存在优先级歧义** | 🟡 中 |
| 5 | §3.2 视频分段 | `segment_duration_seconds: 0` 分段处理为设计能力 | config_parser 解析该字段，**但 runner_video.cpp 中完全未使用**（grep 无 segment 引用）—— 设计存在但**未接线** | 🔴 高（文档描述了不存在的功能行为） |
| 6 | §5.9 断点续传 | Checkpoint 含 `pipeline_state`、`output_manifest`（文件列表+校验和） | 实际 CheckpointData：`task_id/config_hash/last_completed_frame/total_frames/output_path/output_file_size/时间戳/checksum`——**无 pipeline_state、无 output_manifest** | 🟡 中（字段差异） |
| 7 | §5.9 恢复流程 | "校验 checkpoint 完整性(校验和验证)" | 实现有 `calculate_checksum` + `verify_checkpoint` ✅ 一致 | — |
| 8 | §3.2 face_masker | `types: [box, occlusion, region]`，region 支持 18 种 | 实现默认 `region: ["face","eyes"]`（不是 "all"），且 FaceMaskerConfig 无 occluder_model/parser_model 字段 | 🟡 中 |
| 9 | §4.1.4 FrameEnhancer | tile_size/model_scale 参数 | 文档参数 `tile_size [512,512,32]` 在 FrameEnhancerParams 中**无对应字段**（仅 model/enhance_factor） | 🟡 中（Tile 策略可能已内置于实现，需确认） |

---

## 5. 设计层面建议重新评估的点（🔵）

1. **配置级联歧义**: AppConfig.resource.memory_strategy 默认 Strict，TaskConfig.resource.memory_strategy 默认 Tolerant——同一策略两个默认值，MergeConfigs 后以谁为准？文档 §3.1 只写"Task > User > Default"未定义此冲突。
2. **`segment_duration_seconds` 悬空设计**: 字段解析了但 runner 未使用——要么接线实现，要么从文档/配置移除，避免"配置了但无效果"的陷阱。
3. **CLI 参数命名**: `--task-config` vs 文档 `--config`，且快捷模式与 `--task-config` 互斥（`excludes`）但文档 §3.5.1 说"CLI 参数优先级高于配置文件"——互斥设计本身合理，但文档表述与实现策略需统一。

---

## 6. 结论与建议处置顺序

| 优先级 | 处置 | 涉及项 |
|---|---|---|
| P0 | 修正 CLI 参数名文档（`--config` → `--task-config`） | #1 |
| P0 | 决策 `segment_duration_seconds` 去留（接线 or 移除） | #5 |
| P1 | 更新过时标注（FlatBuffers/错误码/配置校验 → 已实现） | §5.8/§5.3.1/§6.1 |
| P1 | 补充缺失字段（default_models、gpu_sample_interval_ms、memory_strategy、max_frames） | #3, #4 |
| P2 | 对齐 Checkpoint 字段描述、face_masker 默认值、FrameEnhancer 参数表述 | #6, #8, #9 |
| P2 | 澄清 CLI 处理器 flag 为动态生成示例 | #2 |
| P3 | 定义配置级联冲突消解规则（memory_strategy 双默认值） | 🔵-1 |

> 本报告仅供审批参考，不直接修改 design.md。审批通过后按上述顺序实施修订。
