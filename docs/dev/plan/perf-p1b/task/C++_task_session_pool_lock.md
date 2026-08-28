# C++ 任务: SessionPool 池锁重构（factory 移出锁）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T3
> **状态**: 已完成（commit `93e6421`）

## 目标

消除 `SessionPool::get_or_create` 持池锁调用 factory 的问题（TRT 引擎加载秒级阻塞全局 session 操作），改为 factory 在锁外执行（double-check + per-key 在建标记）。

## 背景

- `session_pool.cpp:94-131`：`get_or_create` 全程持 `mutex`，factory 内 `load_model`（TRT 引擎构建/加载可达秒级）期间**所有 session 操作阻塞**（评估报告 P-1，P0 级并行放大器）；
- 当前单任务串行下影响有限，但多任务并行（P2-2）时是硬瓶颈；
- 重构目标：锁内仅 map 操作（查/插），factory 在锁外执行。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/foundation/ai/session_pool_test.cpp`（复用现有基建）

用例：
1. `FactoryRunsOutsideLock`：factory 内尝试调用池的 `size()`（会尝试拿锁）→ 不死锁且返回正确 → 证明 factory 执行时锁未持有。
2. `ConcurrentSameKeySingleCreation`：多线程并发 `get_or_create("same_key")` → factory 只调用 1 次（double-check 正确）。
3. `ConcurrentDistinctKeysBothCreated`：多线程并发不同 key → 全部创建成功，无数据竞争（size 正确）。

### 🟢 Green: 实现

- `SessionPool::Impl` 增加 `std::unordered_set<std::string> m_in_flight;`（在建 key 集合，防重复创建）；
- `get_or_create` 重构：
  1. 锁内查缓存 → 命中返回；
  2. 锁内检查 `m_in_flight` → 已在此 key 在建 → **释放锁等待**（condition_variable 通知）→ 重查缓存；
  3. 锁内插入 `m_in_flight` → **释放锁** → 锁外调 factory → 重新拿锁 → 缓存插入 + 移除 `m_in_flight` + notify；
- `evict/clear/cleanup` 与在建 key 的交互：clear 时保留 `m_in_flight` 语义（在建完成后缓存已清空则丢弃）。

### 🔵 Refactor

- 用 `std::condition_variable` + 谓词等待（在建 key 完成通知），避免忙等；
- 异常安全：factory 抛异常时移除 `m_in_flight` 并通知。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `session_pool_tests` 全绿（含新增 3 用例）；现有用例（LRU/TTL/容量）无回归
- [ ] factory 执行期间池可被并发访问（集成观察）

## 提交信息

```bash
perf(session): move session factory outside pool lock with in-flight dedup
```