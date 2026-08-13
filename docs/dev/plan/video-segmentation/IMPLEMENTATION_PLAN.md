# 实施计划：视频分段处理 (Video Segmentation)

## 目标

将 `segment_duration_seconds` 配置字段接线到视频处理流程：按指定秒数将长视频分段处理，最后合并输出为单个文件。实现后消除 design.md V3.0 标注的 WIP 状态。

## 背景

- `config.task.TaskResourceConfig.segment_duration_seconds` 已被 `config_parser` 解析（默认 0 = 不分段）
- `runner_video.cpp` 的 `ProcessVideo` / `ProcessVideoStrict` 目前未使用该字段
- FFmpeg 层已有能力：
  - `VideoReader::seek_by_time(timestamp_ms)` / `seek(frame_index)` — 定位段起点
  - `VideoReader::get_duration_ms()` / `get_frame_count()` / `get_fps()` — 计算段边界
  - `Remuxer::merge_av(video, audio, output)` — 最终音视频合并
  - **无 concat 能力** — 分段合并需自行实现

## 设计

### 方案：按时间窗口循环处理

```
输入视频 (target_path)
  │
  ▼
┌─────────────────────────────────────────────────┐
│ 计算段边界: 段数 = ceil(duration_ms / segment_ms) │
│ 每段 [seg_start_frame, seg_end_frame)            │
└─────────────────────────────────────────────────┘
  │ 对每段:
  ▼
┌──────────────────────────────────────────────┐
│ 1. VideoReader::seek(seg_start_frame)          │
│ 2. 处理该段帧 → 段输出文件 segment_{i}.temp.mp4 │
│    (复用现有 ProcessVideo 帧循环逻辑)          │
│ 3. 记录段文件到清单                             │
└──────────────────────────────────────────────┘
  │ 所有段处理完成
  ▼
┌──────────────────────────────────────────────┐
│ 合并段: 逐段读取 + 写入单一输出文件 (重编码)    │
│ 或: 无音频时直接 concat (流拷贝)              │
└──────────────────────────────────────────────┘
  │
  ▼
最终输出 (output_path) + Remuxer::merge_av 合成音频
```

### 核心决策

| 决策点 | 选择 | 理由 |
|---|---|---|
| 分段粒度 | 按帧索引计算（`frame = duration_seconds × fps`） | VideoReader 有精确 seek(frame)，避免时间戳舍入误差 |
| 段内处理 | 复用现有 `ProcessVideo` 帧循环（提取为内部函数） | 最小改动，保留 Strict/Tolerant 两路径 |
| 段合并方式 | **重编码拼接**：读段文件帧 → 写入最终 VideoWriter | 无 concat 依赖，保证编码参数一致；FFmpeg concat 需要相同 codec 参数，重编码最稳妥 |
| 音频处理 | 与现有逻辑一致：audio_policy=copy 时最终 `Remuxer::merge_av` | 复用现有 mux 路径 |
| Checkpoint 交互 | 分段模式下每段完成后保存 checkpoint | 与现有周期保存共存 |
| 与 resume 交互 | resume 定位到全局帧索引 → 换算到段内偏移 | 需额外处理，见风险 |

### 风险与缓解

| 风险 | 缓解 |
|---|---|
| 段文件占用磁盘空间 | 逐段处理、合并后删除段文件 |
| 段边界帧重复/丢失 | 左闭右开区间 [start, end)，最后一段含末尾 |
| resume + 分段组合复杂度高 | **V1 限制**：`enable_resume=true` 且 `segment_duration_seconds>0` 时记录 WARN 并忽略分段（回退整片处理） |
| strict 模式段间模型卸载重载 | 接受（分段场景显存受限优先），段间日志提示 |
| 帧索引精度 vs 时间 | 使用 `round(seconds × fps)` 换算，误差 < 1 帧 |

### 测试策略

1. **单元/集成测试**（`tests/integration/app/pipeline_runner_video_test.cpp` 新增）：
   - `ProcessVideoSegmentedSmallVideo`：短视频（如 5 秒 slideshow 片段）+ `segment_duration_seconds: 2` → 输出帧数 == 输入帧数，文件存在
   - 使用现有 `slideshow_scaled.mp4`（16.4s, 491 帧）+ segment=4s → 4 段，验证帧数守恒
   - 配置 `audio_policy: skip` 简化（避免 mux 依赖），另测 `copy` 路径
2. **手动验证**：`--config` 运行真实视频，检查输出音画同步

## 实施步骤（TDD）

1. 🔴 新增失败测试：`ProcessVideoSegmented*`（分段配置 → 期望输出完整帧数）
2. 🟢 实现 `ProcessVideoSegmented`（提取公共帧处理逻辑 + 段循环 + 合并）
3. 🔵 重构：复用 Strict/Tolerant 路径，减少重复
4. ✅ 运行 `python build.py --action test --test-label unit` + 集成测试
5. 更新 design.md：移除 WIP 标注，版本 V3.0 → V3.1

## 分支

- 基于当前 `feature/plan-design-doc-revision` 的后续工作？否——**代码功能分支**：从 dev 新建 `feature/plan-video-segmentation`（design 修订分支已提交，先合并回 dev）
