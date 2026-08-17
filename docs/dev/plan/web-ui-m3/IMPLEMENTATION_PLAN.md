# Web UI M3（批量 + 队列优先级）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-M3-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-17

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | 依据设计规格 V0.2.0 创建 M3 实施计划（F3 批量 + F4 队列优先级）。 |

---

**Goal:** 支持文件上传（批量素材）、任务优先级调度与队列位置展示。

**Architecture:** TaskManager 扩展 priority（大优先 + 同优先级 FIFO）；新增 POST /api/upload（二进制 body → temp 目录）；REST 新增优先级修改端点；前端文件多选上传 + 队列/优先级 UI。

**Tech Stack:** Drogon、nlohmann_json、React 19（现有）。

## Global Constraints

- 优先级语义：`priority` int，**数值大优先**，同优先级按提交顺序（FIFO）；默认 0
- 修改优先级仅对 queued 任务生效；running/终态任务返回 409
- 上传：文件落 `temp_directory`（app_config，默认 `./temp`）；文件名消毒（拒绝路径分隔符/..）；大小限制 512MB（M3 简化，不做分片）
- 队列位置计算：queued 任务中按 (priority desc, created_at asc) 排序的序号
- 批量任务（F3）：API 已支持多 source/target 数组（M2 完成）；M3 仅补前端多选上传
- API 契约向后兼容（M2 客户端不破坏）

---

### Task 1: TaskManager 优先级支持

**Files:**
- Modify: `src/app/web/task_types.ixx`（TaskEntry/TaskSummary 加 priority）
- Modify: `src/app/web/task_manager.ixx/.cpp`（submit 带 priority、按优先级取任务、set_priority、queue_position）
- Test: `tests/unit/app/web/task_manager_test.cpp`（扩展）

**接口变更:**
```cpp
std::string submit(config::TaskConfig config, int priority = 0);
/// 修改排队中任务优先级；running/终态返回 false
bool set_priority(const std::string& id, int priority);
```

**TDD 用例:**
- 高优先级任务先于低优先级执行（block executor + 双任务 → running 的是高优先级）
- 同优先级 FIFO
- set_priority 后重新排队（queued 生效）
- set_priority 对 running 返回 false
- 列表包含 priority 与 queue_position

### Task 2: 上传 API + 优先级 REST 端点

**Files:**
- Modify: `src/app/web/web_server.cpp`（/api/upload、/api/tasks/{id}/priority）
- Test: `tests/integration/app/web_api_test.cpp`（扩展）

**API 契约:**
| 方法 | 路径 | 请求 | 响应 |
| :--- | :--- | :--- | :--- |
| POST | /api/upload | 二进制 body；header `X-File-Name` | {path, name, size} |
| POST | /api/tasks/{id}/priority | {priority: N} | {ok, priority} |

**TDD:** 上传成功落盘且返回路径；非法文件名（../）拒绝；priority 修改 queued 生效 / running 409。

### Task 3: 前端批量上传 + 优先级 UI

**Files:**
- Modify: `web/src/api/client.ts`（uploadFile、setPriority）、`web/src/api/types.ts`
- Modify: `web/src/pages/TaskCreatePage.tsx`（文件多选上传 → 路径填入）
- Modify: `web/src/pages/TaskListPage.tsx`（优先级列、队列位置、提升优先级按钮）
- Modify: `web/src/components/FileUploader.tsx`（新建）

**验证:** npm build；dev 模式手动验证上传→提交→列表优先级操作。

### Task 4: e2e + 文档

- e2e（web_api_test.py 扩展）：upload → submit(带上传路径) → set_priority → 列表含优先级
- 文档：web_guide.md（zh/en）补上传与优先级说明；设计文档 M3 标记完成
- 全部测试回归（unit + integration + e2e）

## M3 验收

- [ ] TaskManager 优先级单元测试全绿
- [ ] 上传/优先级集成测试全绿
- [ ] 前端上传与队列 UI build 通过
- [ ] e2e 扩展项通过；文档同步
