# C++ 任务: GPU 并发信号量闸门（max_concurrent_gpu_tasks）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T2
> **状态**: 已完成（commit `6a0cd1d`）

## 目标

实现 `PipelineConfig.max_concurrent_gpu_tasks`（已定义默认 2，从未使用）：Pipeline worker_loop 用 `std::counting_semaphore` 限制并发处理帧数，消除多 worker 抢 GPU 的隐式串行化与拷贝开销。

## 背景

- `pipeline_types.ixx:60`: `int max_concurrent_gpu_tasks = 2;` 字段存在但无任何使用；
- `pipeline_impl.ixx:96-117`: worker_loop 无并发闸门，`worker_thread_count`（默认硬件核/2，常为 4）并发调用 GPU 推理；
- 多个 worker 并发 `Ort::Session::Run` 在 GPU 侧隐式串行 + CPU 拷贝竞争（评估报告 P1-5）；
- 闸门粒度 = worker 处理整帧（含 CPU 预处理与 GPU 推理），是最小侵入方案（processor 接口为黑盒）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/domain/pipeline/` 新增 `pipeline_concurrency_test.cpp`（复用 pipeline_queue_test 基建）

用例：
1. `ConcurrencyLimitedByMaxGpuTasks`：PipelineConfig{worker_thread_count=4, max_concurrent_gpu_tasks=2}，push 8 帧，processor 在 process() 内记录并发峰值（atomic counter + sleep 模拟推理耗时）→ 断言峰值 ≤ 2。
2. `ConcurrencyUnlimitedWhenZero`：max_concurrent_gpu_tasks=0 → 不限制（峰值可达 worker 数）。
3. `OrderPreservedWithGating`：闸门开启时输出仍保序（sequence_id 连续）。

> 说明: 需可观测 processor（计数并发峰值的 FakeProcessor）。

### 🟢 Green: 实现

- `src/domain/pipeline/impl/pipeline_impl.ixx`:
  - `Pipeline` 增加 `std::counting_semaphore<> m_gpu_gate{config.max_concurrent_gpu_tasks > 0 ? config.max_concurrent_gpu_tasks : worker_thread_count}`（0/负值=不限）；
  - `worker_loop` 中每帧处理前 `m_gpu_gate.acquire()`，处理后 `m_gpu_gate.release()`（RAII guard 防异常泄漏）。
- `src/services/pipeline/runner_video.cpp` / `runner_image.cpp`: PipelineConfig 创建处显式设置 `max_concurrent_gpu_tasks = 2`（默认值语义保持，后续可配置化）。

### 🔵 Refactor

- 闸门 guard 用 RAII 封装（scope exit release），异常安全。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `pipeline_concurrency_tests` 全绿；`pipeline_queue_tests` 无回归
- [ ] 集成/E2E 无回归（默认 2 并发下行为正确）

## 提交信息

```bash
perf(pipeline): gate concurrent GPU inference via max_concurrent_gpu_tasks
```