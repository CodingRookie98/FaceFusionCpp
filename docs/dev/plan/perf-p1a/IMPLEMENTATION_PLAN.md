# 性能优化 P1 批次（第一批）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-PERF-P1A-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-28 | AI Agent | 王辉 | 验收完成：单元 308/308、E2E 14/14 全绿；集成 142 例全量跑出现 2 次不同用例的随机 flaky（TaskProgressEndpointWorks / UnknownTaskGetsErrorMessage，均为 Web 时序敏感，单独复现通过），记录为既有测试稳定性问题，非本批回归。 |
| **V1.0.0** | 2026-08-28 | AI Agent | 王辉 | 依据评估报告（[architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)）P1 优先级创建（第一批）：Strict 模式 queue 上限放宽、GPU 并发信号量闸门、session key 文件指纹、TTL 惰性清理。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../../zh/process/workflow.md)
> *   评估报告: [architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块接口（services.pipeline.runner:video / domain.pipeline:impl / foundation.ai.inference_session_registry / foundation.ai.session_pool）。
*   [x] 我已确认新增接口不与现有冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程 (🔴 Red → 🟢 Green → 🔵 Refactor)。

**已检查的上下文:**
*   文件 1: `src/services/pipeline/runner_video.cpp:692` —— Strict 模式 `max_queue_size = min(config, 4)`。
*   文件 2: `src/domain/pipeline/pipeline_types.ixx:60` —— `max_concurrent_gpu_tasks = 2` 已定义未使用；`pipeline_impl.ixx:96-117` worker_loop 无并发闸门。
*   文件 3: `src/foundation/ai/inference_session_registry.cpp:38-54` —— generate_key 无模型文件指纹（热更新失效）。
*   文件 4: `src/foundation/ai/session_pool.cpp:150-173` —— cleanup_expired 存在但无调度者（TTL 形同虚设）。

## 1. 计划概述

### 1.1 目标与范围

1. **P1-1 Strict 模式 queue 上限放宽**: `min(config, 4)` → `min(config, 16)`，缓解流水线吞吐受限。
2. **P1-5 GPU 并发信号量闸门**: Pipeline worker_loop 用 `std::counting_semaphore` 限制并发 GPU 任务数（`PipelineConfig.max_concurrent_gpu_tasks`，默认 2），消除多 worker 抢 GPU 的隐式串行化与拷贝开销；runner 接线设置该字段。
3. **P-4 session key 文件指纹**: `generate_key` 追加模型文件 `size + last_write_time`，解决模型热更新失效。
4. **P-3 TTL 惰性清理**: `SessionPool::get_or_create` 周期（30s）惰性触发 `cleanup_expired`（锁内 internal 版），使 TTL 生效且无线程开销。

*   **涉及模块**: `services.pipeline.runner:video`、`domain.pipeline:impl`、`foundation.ai.inference_session_registry`、`foundation.ai.session_pool`、测试。
*   **范围外**（后续批次）: P1-2 mask 接线+共享（需共享设计确认）、P-1 池锁重构（并行前置）、P2 批次。

### 1.2 成功标准

1. Strict 模式 queue 上限 16（单测/集成观察）。
2. `max_concurrent_gpu_tasks=2` 时 Pipeline 最多 2 个 worker 并发处理帧（单测验证并发计数峰值 ≤ 2）。
3. 模型文件更新（size/mtime 变化）后 `get_session` 返回新 session（单测验证 key 变化）。
4. TTL 到期 session 被惰性清理（单测验证）。
5. 单元测试全绿、集成测试无回归。

## 2. 任务分解

| 任务 | 名称 | 说明 | 依赖 |
| :--- | :--- | :--- | :--- |
| T1 | Strict 模式 queue 上限放宽 | `runner_video.cpp:692` min 上限 4→16 | - |
| T2 | GPU 并发信号量闸门 | Pipeline worker 信号量 + runner 接线 | - |
| T3 | session key 文件指纹 | `generate_key` 加 size+mtime | - |
| T4 | TTL 惰性清理 | SessionPool 周期清理 | - |

> 各任务详细设计、TDD 用例与验收标准见 `task/C++_task_{name}.md`。

## 3. 实施阶段

### 阶段一: 任务文档生成
- [x] 生成 4 个子任务文档（task/C++_task_*.md）

### 阶段二: 分支 + TDD 实现
- [x] 创建分支 `feature/plan-perf-p1a`
- [x] T1 → T4 依次按 🔴 Red → 🟢 Green → 🔵 Refactor 实现
  - T1 `84c4f5d`、T2 `6a0cd1d`、T3 `4e4d311`、T4 `10f5757`
- [x] 每任务提交 + 更新任务文档状态

### 阶段三: 集成验证
- [x] 单元测试全量 308/308 通过
- [x] 集成：全量跑 2 次各有 1 例随机 flaky（Web 时序敏感，单独复现通过，web 子集 19/19 全绿）

### 阶段四: 完成验收与合并
- [x] E2E 测试 14/14 通过
- [x] 合并回 `dev`，删除分支

### 阶段五: 文档归档
- [ ] 更新评估报告（P1/P- 条目标记已修复）