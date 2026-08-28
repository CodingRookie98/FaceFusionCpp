# C++ 任务: SessionPool 容量修正（max_entries 3→10）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T4
> **状态**: 进行中

## 目标

将 `SessionPool::PoolConfig.max_entries` 默认值 3→10，消除单任务（换脸+增强+分割 = 6 session > 3）LRU 驱逐/重建（显存抖动 + TRT 引擎重载秒级延迟）。

## 背景

- `PoolConfig.max_entries` 默认 `3`（`src/foundation/ai/session_pool.ixx:23`）；
- 单任务模型组合：detector + landmarker + swapper + enhancer + occlusion + region = **6 session > 3** → 单任务即触发 LRU 驱逐；
- 驱逐后 session 析构释放显存，重建需重新加载 TRT 引擎（秒级）；
- `configure()` 全项目无调用点（评估报告 §7.1）→ 默认值即实际生效值，改默认即修正实际行为。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/foundation/ai/session_pool_test.cpp`（复用现有基建）

用例：
1. `DefaultCapacitySupportsSixSessions`：默认 `PoolConfig` 构造池，插入 6 个不同 key（mock factory 返回不同 session）→ 断言 `stats.evictions == 0` 且 `size() == 6`（当前默认 3 会驱逐 → 失败）。
2. `ExplicitSmallCapacityStillEvicts`：显式 `max_entries=2` 时 3 个 key → 驱逐 1 次（既有行为回归保护）。
3. `ExplicitLargeCapacityHonored`：显式 `max_entries=20` → 无驱逐。

### 🟢 Green: 实现

- **`src/foundation/ai/session_pool.ixx:23`**: `size_t max_entries{3}` → `size_t max_entries{10}`。
- 现有测试（`session_pool_test.cpp:57-58,97,143` 显式构造 config）不受默认值影响，无需改动。

### 🔵 Refactor

- 无（一行默认值变更；确认无代码依赖默认值 3 的隐性假设，如容量相关测试）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `session_pool_tests` 全绿（含新增 3 用例）；`inference_session_tests` 无回归
- [ ] 单任务 6 模型组合无 LRU 驱逐（集成观察/日志）

## 提交信息

```bash
perf(session): raise SessionPool default capacity to 10 to avoid single-task LRU eviction
```