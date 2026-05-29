# 子任务 1: concurrent_queue.ixx 现代化

## 基本信息
- **所属计划**: fix-tech-debt
- **优先级**: P1
- **修改文件**: `src/foundation/infrastructure/concurrent_queue.ixx`
- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e

## 目标
将 `concurrent_queue.ixx` 中的 `std::lock_guard` 升级为 `std::scoped_lock`，并补齐 Rule of 5 移动语义声明。

## 具体改动
1. **lock_guard → scoped_lock** (6 处，第 71/86/97/108/117/125 行)
   - `std::lock_guard<std::mutex> lock(m_mutex)` → `std::scoped_lock<std::mutex> lock(m_mutex)`
   - `std::scoped_lock` 是 C++17 引入的，支持多锁防死锁，单锁场景下语义更清晰
2. **Rule of 5 移动删除** (在第 29-30 行的拷贝删除后追加)
   - 添加 `ConcurrentQueue(ConcurrentQueue&&) = delete;`
   - 添加 `ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;`
   - 理由：拥有 `std::mutex` 和 `std::condition_variable` 成员（不可移动），应显式声明

## 测试策略
- 编译验证（模块编译必须通过）
- 运行现有 `concurrent_queue_test.cpp`（行为不变，纯重构）

## 验收标准
- [ ] 编译通过
- [ ] 现有单元测试全部通过
- [ ] 无 lock_guard 残留
