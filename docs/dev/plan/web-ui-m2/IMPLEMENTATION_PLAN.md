# Web UI M2（任务闭环 + 基础预览）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-M2-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 依据设计规格 V0.2.0 创建 M2 实施计划（REST 任务闭环 + WebSocket 进度 + 预览）。 |

---

**Goal:** Web 界面可提交/查询/取消任务，实时推送进度，展示结果预览。

**Architecture:** `app.web` 扩展 `TaskManager`（任务注册表 + 后台串行执行线程，执行器可注入以便测试）；Drogon 注册 REST handlers 与 WebSocket 控制器；前端新增任务创建/列表/详情页 + WS 客户端。

**Tech Stack:** Drogon (HTTP/WS)、nlohmann_json（现有）、PipelineRunner（现有，复用）、React 19。

## Global Constraints

- 执行器抽象：`TaskManager` 不直接依赖 `PipelineRunner`，通过 `TaskExecutor`（std::function）注入 —— 单元/集成测试用 fake executor，真实链路用 PipelineRunner
- 任务 id：UUID（core_utils::random::generate_uuid）
- /media/ 文件访问采用**任务内白名单索引**：/media/{task_id}/source/{idx}、/target/{idx}、/result/{filename}，禁止任意路径
- 单任务串行执行（M3 引入队列/优先级）
- 前端产物不提交；API/WS 代理沿用 vite.config.ts
- 状态机: queued → running → done / failed / cancelled

---

### Task 1: TaskManager（任务注册表 + 串行执行）

**Files:**
- Create: `src/app/web/task_manager.ixx/.cpp`
- Create: `src/app/web/task_types.ixx`（TaskStatus/TaskEntry 定义）
- Modify: `src/app/web/CMakeLists.txt`
- Test: `tests/unit/app/web/task_manager_test.cpp`

**Interfaces:**
```cpp
export module app.web.task_manager;
import app.web.task_types;

export namespace app::web {

/// 可注入的执行器: 返回任务结果码 0=成功
using TaskExecutor = std::function<int(const config::TaskConfig&,
                                       const ProgressCallback&)>;

class TaskManager {
public:
    explicit TaskManager(TaskExecutor executor);
    std::string submit(config::TaskConfig config);       // 返回任务 id
    bool cancel(const std::string& id);                  // 取消（running 时调用 runner.cancel）
    std::optional<TaskEntry> get(const std::string& id) const;
    std::vector<TaskSummary> list() const;
    bool is_running() const;
    /// 进度回调注册（供 WebSocket 广播）
    void set_progress_listener(std::function<void(const std::string&, const TaskProgress&)>);
    void shutdown();  // 停止后台线程
};

}
```

**TDD:**
- 🔴 单元测试：submit 生成 id 且状态 queued；fake executor 执行后状态 done；cancel 后状态 cancelled；进度监听收到回调
- 🟢 实现：`TaskEntry{id, status, config, result_files, error_message, progress}`；后台线程循环取队列任务执行
- 集成测试（tests/integration/app/task_manager_api_test.cpp 在 Task 2 后）

### Task 2: REST API

**Files:**
- Modify: `src/app/web/web_server.cpp`（注册 handlers）
- Test: `tests/integration/app/web_api_test.cpp`

**API 契约:**
| 方法 | 路径 | 请求 | 响应 |
| :--- | :--- | :--- | :--- |
| POST | /api/tasks | {source_paths[], target_paths[], output_path, processors[], processor_params{}} | {id, status} |
| GET | /api/tasks | - | [{id, status, progress...}] |
| GET | /api/tasks/{id} | - | {id, status, progress, result_files[], media{...}} |
| POST | /api/tasks/{id}/cancel | - | {ok} |
| GET | /api/tasks/{id}/result | - | {files: [{name, url}]} |

**TDD:** 集成测试：fake executor（立即完成 + 生成输出文件）→ POST/GET/cancel 全链路断言。

### Task 3: WebSocket 进度推送

**Files:**
- Modify: `src/app/web/web_server.cpp`（注册 WebSocketController）
- Create: `src/app/web/ws_controller.ixx/.cpp`（或并入 web_server.cpp）
- Test: `tests/integration/app/web_ws_test.cpp`

**协议:** 端点 `/ws/tasks/{id}/progress`；服务端消息 `{"type":"progress","frame":N,"total":M,"fps":F}`、`{"type":"done"}`、`{"type":"error","message":...}`；连接时推送当前状态。

**TDD:** 集成测试：连接 WS → 提交 fake 任务（进度回调序列）→ 断言收到消息序列。

### Task 4: /media/ 文件访问（白名单）

**Files:**
- Modify: `src/app/web/web_server.cpp`
- Test: `tests/integration/app/web_api_test.cpp`（扩展）

**路由:** `/media/{task_id}/source/{idx}`、`/media/{task_id}/target/{idx}`、`/media/{task_id}/result/{filename}`
- 校验 task 存在；source/target 用白名单索引；result 仅限任务输出目录内的文件
- 越权/不存在 → 404；目录穿越 → 404

### Task 5: 前端任务页

**Files:**
- Create: `web/src/api/client.ts`（REST + WS 封装）、`web/src/api/types.ts`
- Create: `web/src/pages/TaskCreatePage.tsx`、`web/src/pages/TaskListPage.tsx`、`web/src/pages/TaskDetailPage.tsx`
- Modify: `web/src/App.tsx`（路由：react-router-dom 或简单状态切换）
- Create: `web/src/components/ProgressBar.tsx`

**页面:**
- 创建页：source/target 路径输入（多行）、输出路径、processors 勾选、处理器参数（简化：仅 model/参数文本）
- 列表页：任务表格（id/状态/进度），轮询 + WS 刷新
- 详情页：进度条、结果文件列表 + 图片预览（/media/...）、简单左右对比（结果 vs 源图）

**验证:** `npm run build` 通过；dev 模式连真实 ffc --web 手动验证（提交任务需模型）。

### Task 6: e2e + 文档

- e2e（tests/e2e/scripts/web_api_test.py）：ffc --web 启动 → health → 提交任务（expect 失败或取消）→ 列表 → 取消
- 文档：`docs/user/{zh,en}/web_guide.md`（新增：启动方式、页面说明、API 摘要）；设计文档 M2 标记完成
- 全部单元/集成测试回归

## M2 验收

- [ ] POST/GET/cancel 任务闭环可用（fake executor 集成测试全绿）
- [ ] WS 进度推送可用（集成测试）
- [ ] /media/ 白名单访问可用（集成测试）
- [ ] 前端三页面 build 通过，dev 模式可交互
- [ ] 文档同步
