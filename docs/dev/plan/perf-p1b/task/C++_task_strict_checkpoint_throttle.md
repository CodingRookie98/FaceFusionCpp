# C++ 任务: Strict 模式 checkpoint 周期保存

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T4
> **状态**: 进行中

## 目标

Strict 内存模式（默认路径）的 checkpoint 保存改为每 100 帧一次，对齐普通路径（`runner_video.cpp:277`），消除 resume 开启时每帧磁盘 IO。

## 背景

- 普通路径（L277）: `if (ckpt_mgr && seq_id % 100 == 0)` 周期保存；
- Strict 路径（L790-798）: `if (ckpt_mgr) { ... save ... }` **每帧保存**（评估报告 P2-7）；
- `enable_resume` 默认 false，但开启后 Strict 路径每帧写 checkpoint 文件（磁盘 IO 显著）；
- 修复 = 补 `seq_id % 100 == 0` 条件，与普通路径一致。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/services/pipeline/` 新增 `checkpoint_throttle_test.cpp`

用例：
1. `CheckpointSavedEvery100Frames`：模拟 Strict 路径的保存条件（提取为可测纯函数 `should_save_checkpoint(seq_id)` → `seq_id % 100 == 0`）→ 断言 `should_save_checkpoint(1)==false`、`should_save_checkpoint(100)==true`、`should_save_checkpoint(200)==true`。

> 说明: 保存条件提取为 `runner:types` 导出的纯函数 `should_save_checkpoint(int64_t seq_id)`（`seq_id > 0 && seq_id % 100 == 0`），Strict 与普通路径共用。

### 🟢 Green: 实现

- `runner_types.cpp`: 新增 `bool should_save_checkpoint(int64_t seq_id);`
- `runner_video.cpp` Strict 路径（L790-798）: `if (ckpt_mgr && should_save_checkpoint(seq_id))`；普通路径（L277）同步改用该函数（行为一致）。

### 🔵 Refactor

- 两条路径共用 `should_save_checkpoint`，消除重复的 `%100` 魔法数。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `checkpoint_throttle_tests` 全绿；checkpoint_manager 相关测试无回归
- [ ] Strict 路径 checkpoint 每 100 帧保存（集成观察）

## 提交信息

```bash
perf(video): throttle strict-mode checkpoint saving to every 100 frames
```