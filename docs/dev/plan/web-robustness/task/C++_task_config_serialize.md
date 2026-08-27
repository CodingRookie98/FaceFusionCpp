# C++ 任务: TaskConfig/TaskEntry JSON 序列化

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 阶段一（3.1）
> **状态**: 进行中

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

- **config 层**（`src/app/config/config_serialize.ixx/.cpp`，或并入 config_merger 同层新模块）:
  - `Result<nlohmann::json, ConfigError> SerializeTaskConfig(const TaskConfig&)`
  - `Result<TaskConfig, ConfigError> DeserializeTaskConfig(const nlohmann::json&)`
  - 枚举辅助：`to_string(FaceSelectorMode)` 等（若已存在则复用 config_parser 的映射，避免重复）。
- **app.web 层**（`src/app/web/task_types.ixx` 或新 `task_serialize.ixx/.cpp`）:
  - `nlohmann::json TaskEntryToJson(const TaskEntry&)`
  - `Result<TaskEntry, std::string> TaskEntryFromJson(const nlohmann::json&)`

### 🔵 Refactor

- 复用 config_parser 现有枚举映射（如存在），消除重复。
- 保持纯函数、无 IO 副作用（文件读写不在本任务）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `python build.py --action test --test-label unit` 新增用例全绿
- [ ] 覆盖全部 4 种 processor variant

## 提交信息

```bash
feat(config): add TaskConfig/TaskEntry JSON serialization for task persistence
```