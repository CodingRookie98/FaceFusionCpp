# C++ 任务: SessionPool TTL 惰性清理

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T4
> **状态**: 进行中

## 目标

让 `SessionPool` 的 TTL 过期清理真正生效：`get_or_create` 周期（30s）惰性触发清理，替代当前无调度者的 `cleanup_expired()`。

## 背景

- `session_pool.cpp:150-173`（cleanup_expired）: 实现完整但**全项目无调用点**（评估报告 §7.1/P-3）→ 空闲 session 永不释放；
- TTL 配置 `idle_timeout` 默认 60s；
- 方案：惰性清理（每次 `get_or_create` 时检查距上次清理是否 ≥30s → 触发）——零线程、无额外调度器。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/foundation/ai/session_pool_test.cpp`（复用现有基建）

用例：
1. `LazyCleanupExpiresIdleSessions`：`PoolConfig{idle_timeout=50ms}`，插入 key1 → sleep 100ms → 再次 `get_or_create("key_new", factory)` 触发惰性清理 → 断言 key1 被移除（`size()` 不含 key1）、`stats.expirations >= 1`。
2. `LazyCleanupKeepsFreshSessions`：`idle_timeout=10s`，插入 key1 → 立即 `get_or_create("key2")` → key1 仍在（未过期）→ `size() == 2`。
3. `CleanupIntervalThrottles`：清理间隔 30s 内多次 get_or_create 不重复触发（内部 `last_cleanup` 时间戳断言或行为等价）。

> 说明: 惰性清理间隔（30s）应可注入（构造参数或内部常量）以便测试。

### 🟢 Green: 实现

- `src/foundation/ai/session_pool.cpp`:
  - `Impl` 增加 `std::chrono::steady_clock::time_point m_last_cleanup{}`；
  - `get_or_create` 锁内开头：`now - m_last_cleanup >= cleanup_interval` → 调 `cleanup_expired_internal()`（新拆出的无锁版）并更新 `m_last_cleanup`；
  - `cleanup_expired()` 保留（public），内部调 `cleanup_expired_internal()`（加锁）。

### 🔵 Refactor

- 清理逻辑拆 `cleanup_expired_internal()`（假设锁已持有），避免 get_or_create 内重复加锁（非递归 mutex 死锁防护）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `session_pool_tests` 全绿（含新增 3 用例）；现有 TTLExpiration 用例保持通过
- [ ] 空闲 session 按 TTL 自动释放（集成观察）

## 提交信息

```bash
perf(session): lazy TTL cleanup on get_or_create so idle sessions expire
```