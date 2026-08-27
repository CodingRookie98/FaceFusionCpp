# C++ 任务: TaskManager 任务超时机制

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 阶段三（3.3）
> **状态**: 进行中

## 目标

为运行中的任务增加超时检测：超过 `max_execution_seconds` 后自动取消并标记 failed，保留已产出部分帧。

## 背景

- 现状：任务可永久运行（无超时机制），模型推理异常时任务卡死无法终止。
- 依赖：阶段二引入的 `TaskManagerOptions.max_execution_seconds`。

## 设计

- worker_loop 执行任务前记录 `started_at = steady_clock::now()`。
- 执行期间用 condition_variable `wait_for(1s)` 周期唤醒检查：
  - 若 `now - started_at >= max_execution_seconds` → 调用 `executor->cancel()` → 任务标记 `Failed`，error_message 含 "timeout"。
  - 否则继续等待（同时保持对 shutdown 的响应）。
- 超时不清理输出目录（保留已产出部分帧）。
- `max_execution_seconds <= 0` 视为禁用超时（无限等待，兼容测试/特殊场景）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/app/web/task_manager_timeout_test.cpp`

用例：
1. **超时触发**：FakeExecutor block（永不返回）+ `max_execution_seconds=200ms` → 轮询等待 → 任务状态变为 failed，error_message 含 "timeout"，executor->cancel 被调用。
2. **正常任务不受影响**：FakeExecutor 快速完成 + 同样超时配置 → 任务 done，不误杀。
3. **超时禁用**：`max_execution_seconds=0` + block executor → 任务保持 running（配合 shutdown 结束测试，不无限等待）。
4. **超时后保留产物**：超时场景中 FakeExecutor 先写一个 dummy 文件 → 超时后输出目录文件仍在。

### 🟢 Green: 实现

- `task_manager.cpp` worker_loop：在执行块中加入超时检测循环（wait_for + 时长检查）。
- 超时路径复用现有状态迁移逻辑（标记 failed + status_listener 通知 + 快照更新）。

### 🔵 Refactor

- 超时检测与正常执行完成逻辑分离为清晰分支。
- 确保 shutdown 时超时等待立即退出（wait_for 与 stop_cv 协同）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 超时触发/不误杀/禁用三种场景全绿
- [ ] 超时后部分产物保留

## 提交信息

```bash
feat(web): enforce per-task execution timeout in task manager
```