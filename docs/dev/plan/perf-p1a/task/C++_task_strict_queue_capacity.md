# C++ 任务: Strict 模式 queue 上限放宽

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T1
> **状态**: 已完成（commit `84c4f5d`）

## 目标

Strict 内存模式下 Pipeline 帧队列上限从 4 放宽到 16，缓解 reader 频繁阻塞导致的 GPU 空闲窗口。

## 背景

- `runner_video.cpp:692`（Strict 路径）: `max_queue_size = std::min(task_config.resource.max_queue_size, 4)`；
- 普通路径（L158）与分段路径（L436）无此限制（直接用 config）；
- Strict 是默认 `memory_strategy`，queue=4 是默认路径的吞吐瓶颈之一（评估报告 P1-1）；
- 队列缓冲为 CPU 内存（帧 cv::Mat），放宽到 16 增加内存有限（16×6MB@1080p ≈ 96MB），换取流水线吞吐。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/services/pipeline/` 新增 `strict_queue_capacity_test.cpp` 或复用 runner 测试基建

用例：
1. `StrictModeQueueCapSixteen`：构造 Strict 路径的 pipeline_config 计算逻辑（提取为可测 helper `strict_queue_limit(config)`）→ 断言 `strict_queue_limit({max_queue_size=100}) == 16`、`strict_queue_limit({max_queue_size=8}) == 8`。

> 说明: 若直接修改 runner_video.cpp 的常量，可测性差。将上限计算提取为 `runner:video` 导出的纯函数 `strict_queue_limit(int configured)`（min(config, 16)）。

### 🟢 Green: 实现

- `src/services/pipeline/runner_video.cpp`:
  - 新增 `int strict_queue_limit(int configured)`（`min(configured, 16)`，`configured<=0` 时返回 16）；
  - L692 改为 `pipeline_config.max_queue_size = strict_queue_limit(task_config.resource.max_queue_size);`。

### 🔵 Refactor

- 常量 16 提取为命名常量（如 `kStrictQueueCap = 16`），与注释说明内存/吞吐权衡。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `strict_queue_limit` 相关测试全绿；既有 runner 测试无回归
- [ ] Strict 模式视频处理 queue 上限 16（集成观察）

## 提交信息

```bash
perf(video): raise strict-mode pipeline queue cap from 4 to 16
```