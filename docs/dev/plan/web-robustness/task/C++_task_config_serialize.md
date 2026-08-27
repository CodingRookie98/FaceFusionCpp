# C++ 任务: TaskConfig/TaskEntry JSON 序列化

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 阶段一（3.1）
> **状态**: 已完成

## 目标

实现 config 层 `TaskConfig` 与 app.web 层 `TaskEntry` 的 JSON 序列化/反序列化，为任务持久化提供基础能力。

## 背景

- `TaskConfig`（`src/app/config/task_config.ixx`）含嵌套结构（TaskInfo/IOConfig/OutputConfig/TaskResourceConfig/FaceAnalysisConfig/PipelineStep）与 `StepParams` variant、多个枚举。
- 现有代码仅 web_server.cpp 有 JSON 输出辅助（task_summary_to_json 等），无完整 TaskConfig 序列化。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/app/config/config_serialize_test.cpp`

用例：
1. `SerializeTaskConfig` 往返一致性：构造含全部 4 种 processor 的 TaskConfig → serialize → deserialize → 字段逐一相等（含 variant 内容）。
2. variant 分发：face_swapper（model+selector_mode+reference_face_path）→ JSON 结构含对应字段。
3. 枚举字符串化：`FaceSelectorMode::Reference` ↔ `"reference"`、`ConflictPolicy::Overwrite` ↔ `"overwrite"`、`AudioPolicy::Copy` ↔ `"copy"`、`ExecutionOrder::Sequential` ↔ `"sequential"`、`MemoryStrategy::Strict` ↔ `"strict"`。
4. optional 空值：`reference_face_path` 为空时序列化为 null / 反序列化为 nullopt。
5. 非法 JSON 反序列化返回错误（不抛异常）。

**文件**: `tests/unit/app/web/task_entry_serialize_test.cpp`

用例：
1. TaskEntry ↔ JSON 往返：id/status/progress/error_message/result_files/created_at/priority。
2. TaskStatus 字符串映射：5 种状态。

### 🟢 Green: 实现

- **config 层**（`src/app/config/parser/config_parser.ixx/.cpp`，复用既有枚举转换）:
  - `Result<nlohmann::json> SerializeTaskConfig(const TaskConfig&)`
  - `Result<TaskConfig> DeserializeTaskConfig(const nlohmann::json&)`
  - 枚举转换复用 config_parser 既有 `to_string`/`parse_*` API，无重复。
- **app.web 层**（TaskEntry ↔ JSON）: **并入阶段二**（作为 TaskManager 持久化的内部辅助，随快照读写一起实现与测试）。

### 🔵 Refactor

- 复用 config_parser 现有枚举映射，消除重复。
- 保持纯函数、无 IO 副作用（文件读写不在本任务）。
- 反序列化严格性：缺 `config_version` 或 `pipeline` 类型错误 → 返回错误（损坏快照可被检测）。

## 验收标准

- [x] 失败测试先行编写（Red 确认）
- [x] 编译通过（无警告）
- [x] `config_serialize_tests` 5/5 全绿；`config_parser_tests` 19/19、`config_merger_tests` 10/10 无回归
- [x] 覆盖全部 4 种 processor variant

## 提交信息

```bash
feat(config): add TaskConfig/TaskEntry JSON serialization for task persistence
```