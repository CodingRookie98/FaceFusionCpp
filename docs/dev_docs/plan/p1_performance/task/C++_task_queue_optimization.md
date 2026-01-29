<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的“物理布局”和“接口设计”。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
-->

# ThreadSafeQueue 优化任务单

> **标准参考 & 跨文档链接**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../C++_quality_standard.md)
> *   所属计划: [P1 性能优化计划](../C++_plan_performance_optimization.md)
> *   相关评估: [链接至对应评估](../evaluation/C++_evaluation_queue_optimization.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [x] **父计划**: 我已阅读 `docs/dev_docs/plan/p1_performance/C++_plan_performance_optimization.md` 并理解架构。
*   [x] **依赖检查**: 我已确认 `src/domain/pipeline/queue.ixx` 是 `ThreadSafeQueue` 定义的位置。

## 1. 任务概览

### 1.1 目标
> @brief 优化 `ThreadSafeQueue` 性能，引入批量操作接口 (`pop_batch`, `push_batch`) 以减少锁获取频率，从而提升高吞吐场景下的并发性能。

*   **目标**: 减少锁竞争，提升队列吞吐量。
*   **所属计划**: [P1 性能优化计划](../C++_plan_performance_optimization.md)
*   **优先级**: Medium

### 1.2 模块变更清单 (关键)

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Queue Implementation** | `src/domain/pipeline/queue.ixx` | Modify | 添加 `pop_batch` 接口 |
| **Pipeline Implementation** | `src/domain/pipeline/impl/pipeline_impl.ixx` | Modify | 更新 Worker 使用批量接口 |

## 2. 详细设计与实现

### 2.1 接口扩展 (Queue)

```cpp
// src/domain/pipeline/queue.ixx

export template <typename T>
class ThreadSafeQueue {
public:
    // Existing methods...

    /**
     * @brief Pop multiple items at once to reduce lock contention
     * @param max_items Maximum number of items to retrieve
     * @return Vector of items (empty if queue was empty or stopped)
     */
    std::vector<T> pop_batch(size_t max_items) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_not_empty.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });

        std::vector<T> batch;
        if (m_shutdown && m_queue.empty()) return batch;

        size_t count = std::min(max_items, m_queue.size());
        batch.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            batch.push_back(std::move(m_queue.front()));
            m_queue.pop();
        }

        m_cond_not_full.notify_all(); // Notify producers
        return batch;
    }
};
```

### 2.2 消费者逻辑更新 (Worker)

```cpp
// src/domain/pipeline/impl/pipeline_impl.ixx

void worker_loop() {
    while (m_active) {
        // Fetch batch of up to 4 frames
        auto frames = m_input_queue.pop_batch(4);
        if (frames.empty() && !m_input_queue.is_active()) break;

        for (auto& frame : frames) {
            // Process frame...
        }
    }
}
```

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: 确保模板代码无语法错误。
*   [ ] **逻辑验证**: 确保 Batch 操作不会丢失数据，且 Shutdown 信号能正确传递。
*   [ ] **性能对比**: 运行 Benchmark，观察 FPS 变化。

---
**执行人**: Sisyphus
**开始日期**: 2026-01-29
