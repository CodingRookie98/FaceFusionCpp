<!-- AI_CONTEXT
你是一名高级 C++ 工程师。目标：实施父计划中定义的任务，且不产生副作用。
约束：严格遵循父计划的物理布局与接口设计，不在此引入新的架构模式。
TDD 强制：先写失败测试（Red）→ 最小实现（Green）→ 测试保护下重构（Refactor）。
-->

# {任务名称} 实施任务单

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-TASK-{任务简称}-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 草案 (Draft)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: YYYY-MM-DD

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | YYYY-MM-DD | AI Agent | 王辉 | 初始任务单。 |

---

> **关联**
> * 所属计划：`docs/project/plans/{plan_name}/IMPLEMENTATION_PLAN.md`
> * 工作流程：[workflow.md](../reference/zh/rules/workflow.md)（命令、失败阈值、产物路径）
> * 质量标准：[C++_quality_standard.md](../reference/zh/rules/C++_quality_standard.md)

## 目标

一句话说明本任务的具体产出（例：实现 `Order` 模块的 Pricing 分区）。

## 背景

- **现状/痛点**：{附代码位置 `file:line`，说明为何需要改}
- **依赖与前置**：{依赖的任务、模块或数据}

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**：新增或修改 `tests/{层级}/{模块}/xxx_test.cpp`

用例：

1. `CaseName`：{输入} → {期望断言}
2. `CaseName`：{边界/异常输入} → {期望行为}

> 说明：{测试缝隙选择与替身策略；纯逻辑用单元测试，GPU/模型依赖用集成测试}

### 🟢 Green: 实现

- **改动范围**：`file1`、`file2`（**锁定 diff 范围，不做顺带重构**）
- **关键实现**：{最小可用实现要点}

### 🔵 Refactor

- 在测试保护下消除重复、扁平化结构；重构后既有测试必须全绿。

## 验收标准

- [ ] 失败测试先行编写（Red 已确认）
- [ ] 单元测试全绿：`python build.py --action test --test-label unit`
- [ ] 编译无新增告警
- [ ] 跨模块/跨层改动：集成测试通过（`--test-label integration`）
- [ ] 契约/配置变更：相关文档已同步

## 提交信息

```bash
<type>(<scope>): <summary>
```
