# Web 任务持久化与超时机制 实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEB-ROBUSTNESS-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-27

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-27 | AI Agent | 王辉 | 依据 Web 功能设计评估（[web_ui_design.md](../../zh/architecture/web_ui_design.md) V0.7.0 §8.2）创建：改进项 1（任务持久化）+ 改进项 2（任务超时机制）。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../zh/process/workflow.md)
> *   设计规格: [web_ui_design.md](../../zh/architecture/web_ui_design.md)

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块接口（app.web.task_manager / task_types / config.app / config.parser）。
*   [x] 我已确认新增接口不与现有冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程 (🔴 Red → 🟢 Green → 🔵 Refactor)。

**已检查的上下文:**
*   文件 1: `src/app/web/task_manager.ixx/.cpp` —— TaskManager 构造仅接受 executor；任务状态机 Queued/Running/Done/Failed/Cancelled；worker_loop 串行执行；任务纯内存存储。
*   文件 2: `src/app/web/task_types.ixx` —— TaskEntry/TaskSummary/TaskProgress 定义（无序列化）。
*   文件 3: `src/app/config/app_config.ixx` —— WebConfig 现有字段 host/port/web_root/temp_dir。
*   文件 4: `src/app/config/parser/config_parser.cpp` —— web 节解析（host/port/web_root/temp_dir），需扩展。
*   文件 5: `src/app/cli/app_cli.cpp:330` —— `make_shared<TaskManager>(executor)`，接线点。
*   文件 6: `tests/unit/app/web/task_manager_test.cpp` —— 现有 FakeExecutor 测试基建（block/cancel/emit_progress/create_dummy_result）。
*   文件 7: `config/app.yaml` —— web 节现有 4 字段。

## 1. 计划概述

### 1.1 目标与范围

*   **核心目标**（对齐设计规格 §8.2 改进项 1+2）:
    1. **任务持久化**: TaskManager 任务状态与配置落盘 JSON 快照；服务重启后自动恢复（queued→恢复排队，running→标记 failed 如实反映，终态保留为历史）。
    2. **任务超时机制**: 单任务最长执行时间可配置（`web.max_execution_seconds`，默认 3600s=1h）；超时后任务标记 failed 并保留已产出部分帧。

*   **涉及模块**: `app.web.task_manager`、`app.web.task_types`、`config.app`（WebConfig）、`config.parser`、`app.cli`、测试。
*   **范围外**（本计划不实施，见设计规格 §8.2/§8.3）: 队列上限（改进项 3）、鉴权（改进项 6）、任务命名（改进项 4）、OpenAPI（改进项 5）、前端改进（7/8）。

### 1.2 关键约束

*   [x] **标准**: C++20，模块化（`.ixx` 接口 + `.cpp` 实现）。
*   [x] **分层**: 序列化函数放 config 层（config.merger 同层），TaskManager 调用，避免 app.web → config 反向依赖。
*   [x] **构建**: `python build.py`（Debug 默认）。
*   [x] **兼容**: TaskManager 构造函数保持向后兼容（新参数带默认值），现有测试不破坏。

## 2. 架构设计

### 2.1 任务持久化方案

**快照文件格式**: `{persist_dir}/{task_id}.json`（每任务一个文件，JSON 快照含完整 TaskEntry 序列化）。

**序列化实现**:
*   新增 `config::SerializeTaskConfig` / `DeserializeTaskConfig`（config 层，与 merger 同层，可 GTest 单测）：
    *   TaskConfig 序列化为 JSON（task_info / io / resource / face_analysis / pipeline）。
    *   pipeline 中 `StepParams` variant 按 `step` 类型字段分发（face_swapper/face_enhancer/expression_restorer/frame_enhancer）。
    *   枚举（FaceSelectorMode/ConflictPolicy/AudioPolicy/ExecutionOrder/MemoryStrategy）序列化为字符串。
*   新增 `app.web` 层 TaskEntry ↔ JSON 序列化辅助（复用 config 层 TaskConfig 序列化 + 状态/进度/时间戳）。

**持久化时机**（TaskManager 内部）:
*   `submit()`: 写快照（queued 态）。
*   状态迁移（running→done/failed/cancelled）: 更新快照。
*   `cancel()` / `set_priority()`: 更新快照。

**恢复语义**（TaskManager 构造时，若 persist_dir 非空）:
*   扫描 persist_dir 下 `*.json`，反序列化全部 TaskEntry。
*   `Queued` → 重新入队（恢复排队执行）。
*   `Running` → 标记 `Failed`（error_message="Task interrupted by service restart"），保留结果文件。
*   `Done/Failed/Cancelled` → 作为历史保留（不重新执行）。

**失败防御**: 单个快照损坏（JSON 解析失败）跳过该文件并告警，不阻塞整体恢复。

### 2.2 任务超时机制

*   TaskManager 新增构造参数 `max_execution_seconds`（默认 3600）。
*   worker_loop 执行前记录 `started_at`；执行期间周期检查（如每 1s）已运行时长，超过上限则调用 `executor->cancel()` 并标记 `Failed`（error_message 注明超时）。
*   保留已产出部分帧（不清理输出目录）。
*   超时检查通过 condition_variable `wait_for` 实现，避免忙轮询。

### 2.3 配置接线

*   `WebConfig` 新增字段:
    *   `std::string persist_dir = "./temp/tasks";`
    *   `int max_execution_seconds = 3600;`
*   `config_parser.cpp` web 节解析两字段。
*   `app_cli.cpp` run_web_mode: `make_shared<TaskManager>(executor, {.persist_dir = app_config.web.persist_dir, .max_execution_seconds = app_config.web.max_execution_seconds})`。
*   `config/app.yaml` web 节追加两字段（含注释）。

## 3. 实施路线图

### 3.1 阶段一: TaskConfig/TaskEntry JSON 序列化（TDD）

**目标**: config 层 TaskConfig 序列化 + app.web 层 TaskEntry 序列化，全部可单测。

*   [ ] **任务 1.1**: 编写失败测试 `SerializeTaskConfig`/`DeserializeTaskConfig`（config_serialize_test.cpp）：往返一致性、variant 分发、枚举字符串化、optional 空值。
*   [ ] **任务 1.2**: 实现 config 层序列化函数 → 对应 Task: [C++_task_config_serialize.md](./task/C++_task_config_serialize.md)
*   [ ] **任务 1.3**: 编写失败测试 + 实现 TaskEntry ↔ JSON（task_entry_serialize_test.cpp）：状态/进度/时间戳/结果文件。
*   [ ] **验收标准**: 🔴 失败测试先行；编译通过（无警告）；单元测试全部通过。

### 3.2 阶段二: TaskManager 持久化集成（TDD）

**目标**: submit 落盘、状态迁移更新、启动恢复。

*   [ ] **任务 2.1**: 编写失败测试（task_manager_persist_test.cpp）：submit 生成快照文件；模拟重启（新 TaskManager 同 persist_dir）queued 恢复、running→failed、终态保留；损坏快照跳过。
*   [ ] **任务 2.2**: TaskManager 集成持久化（构造恢复 + submit/状态迁移落盘）→ 对应 Task: [C++_task_persistence.md](./task/C++_task_persistence.md)
*   [ ] **验收标准**: 🧪 单元测试全绿；模拟重启场景测试通过。

### 3.3 阶段三: 任务超时机制（TDD）

**目标**: 超时检测与处理。

*   [ ] **任务 3.1**: 编写失败测试（task_manager_timeout_test.cpp）：FakeExecutor block + 短超时（如 200ms）→ 任务自动 failed、error_message 含 timeout、cancel 被调用。
*   [ ] **任务 3.2**: worker_loop 集成超时检测 → 对应 Task: [C++_task_timeout.md](./task/C++_task_timeout.md)
*   [ ] **验收标准**: 🧪 单元测试全绿；非超时任务不受影响。

### 3.4 阶段四: 配置接线与集成验证

*   [ ] **任务 4.1**: WebConfig + config_parser + app.yaml 追加字段 → 对应 Task: [C++_task_config_wiring.md](./task/C++_task_config_wiring.md)
*   [ ] **任务 4.2**: app_cli 接线 TaskManager 新参数。
*   [ ] **任务 4.3**: 集成测试 `python build.py --action test --test-label integration`（web_api_test 增补持久化/超时场景或手动验证）。
*   [ ] **验收标准**: 完整测试套件通过；手动验证：提交任务→重启 ffc→任务恢复/标记 failed。

## 4. 风险管理

| 风险点 | 可能性 | 影响 | 缓解措施 |
| :--- | :--- | :--- | :--- |
| TaskConfig variant 序列化遗漏类型 | 中 | 反序列化失败 | 单元测试覆盖全部 4 种 processor |
| 快照损坏导致恢复崩溃 | 中 | 服务启动失败 | 单文件失败跳过 + 告警，不阻塞 |
| running 任务恢复后重复处理 | 中 | 重复消费素材 | 恢复语义 running→failed（如实标记，不假续跑） |
| 超时检测误杀长任务 | 低 | 任务失败 | 默认 1h 可配置；仅对 running 任务检测 |
| 持久化目录不可写 | 低 | submit 失败 | 写快照失败仅告警，不阻塞任务提交（内存优先） |

## 5. 资源与依赖

*   **外部依赖**: 无新增（nlohmann_json 现有）。
*   **前置任务**: 无。
*   **关键工具链**: CMake + Ninja、GCC/Clang C++20、`python build.py`、Google Test。

## 6. 分支与提交

*   分支：`feature/plan-web-robustness`（基于 dev）。
*   提交顺序：Task 1.1-1.3（序列化）→ Task 2.x（持久化）→ Task 3.x（超时）→ Task 4.x（配置接线），每任务独立 commit。
*   测试分层（AGENTS.md）: C++ 单模块改动 + 跨模块（config→web），单元测试 MUST、集成测试 MUST（后台）。