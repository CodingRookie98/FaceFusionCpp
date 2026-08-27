# C++ 任务: TaskManager 任务持久化集成

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 阶段二（3.2）
> **状态**: 已完成

## 目标

TaskManager 集成任务持久化：submit 落盘 JSON 快照、状态迁移更新快照、构造时从 `persist_dir` 恢复任务。

## 背景

- 现状：任务纯内存存储（`task_manager.cpp` 中 `tasks` map），服务重启全部丢失。
- 依赖：阶段一已实现的 TaskConfig/TaskEntry 序列化函数。

## 设计

### 构造签名（向后兼容）

```cpp
struct TaskManagerOptions {
    std::string persist_dir = "";       ///< 空 = 不持久化（默认，保持旧行为）
    int max_execution_seconds = 3600;   ///< 超时（阶段三使用）
};
explicit TaskManager(std::shared_ptr<ITaskExecutor> executor,
                     TaskManagerOptions options = {});
```

### 快照文件

- 路径：`{persist_dir}/{task_id}.json`
- 内容：TaskEntry 完整序列化（含 TaskConfig）。

### 持久化时机

| 时机 | 动作 |
|---|---|
| `submit()` | 写快照（queued 态） |
| 状态迁移 running→done/failed/cancelled | 更新快照 |
| `cancel()` / `set_priority()` | 更新快照 |

### 恢复逻辑（构造时，persist_dir 非空）

1. 扫描 persist_dir 下 `*.json`，逐个反序列化。
2. 状态映射：
   - `Queued` → 重新入队（恢复排队执行）。
   - `Running` → 标记 `Failed`，error_message = "Task interrupted by service restart"。
   - `Done/Failed/Cancelled` → 保留为历史，不重新执行。
3. 损坏快照：跳过该文件 + Logger 告警，不阻塞整体恢复。

### 写快照失败处理

- 写失败仅 Logger 告警，不阻塞任务提交（内存优先，持久化为尽力而为）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/app/web/task_manager_persist_test.cpp`

用例：
1. submit 后 persist_dir 下生成 `{task_id}.json` 文件。
2. 模拟重启：创建 TaskManager A（persist_dir=X）→ submit queued 任务 → 销毁 A → 创建 TaskManager B（同 persist_dir）→ queued 任务恢复排队且可执行。
3. running→failed：submit + FakeExecutor block → 销毁（running 态）→ 重建 → 原任务状态为 failed，error_message 含 "restart"。
4. 终态保留：submit → 完成（done）→ 重建 → 任务仍为 done 且不重新执行。
5. 损坏快照：persist_dir 放入非法 JSON 文件 → 重建不崩溃，其他任务正常恢复。
6. persist_dir 为空（默认）→ 不产生任何快照文件（旧行为保持）。

### 🟢 Green: 实现

- `task_manager.ixx`: 新增 `TaskManagerOptions` 与构造重载。
- `task_manager.cpp`:
  - Impl 增加 `persist_dir`、`save_snapshot(id)`、`load_snapshots()` 私有方法。
  - `submit()`: 落盘。
  - worker_loop 状态迁移处: 更新快照。
  - `cancel()`/`set_priority()`: 更新快照。
  - 构造函数（Impl 构造）: 若 persist_dir 非空 → `load_snapshots()`。

### 🔵 Refactor

- 快照读写封装为独立私有方法，保持 worker_loop 可读性。
- 与阶段三的超时检测互不干扰。

## 验收标准

- [x] 失败测试先行编写（Red 确认）
- [x] 模拟重启场景测试全绿（7 用例）
- [x] 损坏快照不崩溃（跳过 + 告警）
- [x] 默认 persist_dir 空时旧行为保持（无快照产生）
- [x] running 任务置 Running 时落盘（真实崩溃恢复语义正确）
- [x] 既有 task_manager_test 13 用例无回归

## 提交信息

```bash
feat(web): persist task snapshots to disk and restore on restart
```