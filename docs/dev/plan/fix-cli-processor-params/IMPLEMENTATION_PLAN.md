# CLI 处理器参数失效修复 实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-CLI-PARAMS-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-14 | AI Agent | 王辉 | 全部任务完成：单元测试（含 6 个新用例）与集成测试通过；格式化通过；clang-tidy 因模块环境超时跳过。 |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 依据 CLI 功能评估报告创建修复计划。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../zh/process/workflow.md)
> *   相关评估报告: CLI 功能完整性评估报告（会话内交付）

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块的接口（config.merger / config.parser / processor.param_registry / app.cli）。
*   [x] 我已确认建议的函数名不与现有的冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程 (🔴 Red → 🟢 Green → 🔵 Refactor)。

**已检查的上下文:**
*   文件 1: `src/app/cli/app_cli.cpp`（build_quick_task_config 仅写 `step.cli_params`，全项目无消费点）
*   文件 2: `src/app/config/config_merger.cpp`（ApplyDefaultModels 只处理 `step.params`）
*   文件 3: `src/services/pipeline/pipeline_runner.cpp`（Runner 只读 `step.params`，frame_enhancer 硬编码 `real_esrgan_x4_plus` 不存在于 models_info.json）
*   文件 4: `src/domain/pipeline/pipeline_adapters.cpp`（注册表 `default_value` 字段从未被消费）
*   文件 5: `tests/e2e/scripts/test_cli_flags.py`（使用未实现的 `--headless` 标志，必然失败）
*   文件 6: `docs/user/zh/cli_reference.md`（默认值列与 app_config 实际默认不符）

## 1. 计划概述

### 1.1 目标与范围

*   **核心目标**: 修复 CLI 功能评估发现的 3 个缺陷：
    1. 快捷模式处理器参数（`--face-swapper-model` 等 15 个标志）解析成功但静默失效；
    2. e2e 测试 `test_cli_flags.py` 使用未实现的 `--headless` 标志必然失败；
    3. 默认模型名三处不一致（文档/注册表/app_config/runner 硬编码），注册表 `default_value` 为死数据。
*   **涉及模块**: `app.cli`、`config.merger`、`config.task`、`processor.param_registry`、`services.pipeline`、测试与用户文档。

### 1.2 关键约束

*   [x] **标准**: C++20，模块化（`.ixx` 接口 + `.cpp` 实现）。
*   [x] **分层**: config 层不反向依赖 domain 层（故 default_value 兜底方案不引入 config→domain 依赖）。
*   [x] **构建**: `python build.py`（Debug 默认）。

## 2. 架构设计

### 2.1 缺陷 1 修复方案（核心）

**现状链路（断裂）**:
```
CLI 标志 → register_processor_cli_params() → processor_params map
        → build_quick_task_config() 写入 step.cli_params  ← 死胡同，无消费点
```

**修复后链路**:
```
CLI 标志 → processor_params map → step.cli_params
        → 新增 ApplyCliParamsToStep(step)（config.merger 层，可单测）
        → step.params（typed variant，与 YAML 路径汇合）
        → MergeConfigs 默认值合并 → PipelineRunner 消费
```

**设计决策**:
*   新函数 `config::ApplyCliParamsToStep(PipelineStep&)` 放入 `config_merger.ixx/.cpp`：与现有 `ApplyDefaultModels` 同层、可复用 `parse_face_selector_mode`、便于 GTest 直接单测。
*   转换规则参照 `config_parser.cpp ParsePipelineStep`（YAML 解析模式），覆盖 4 种处理器的全部参数：model/face_selector_mode/reference_face_path/blend_factor/restore_factor/enhance_factor。
*   数值转换（`blend_factor` 等）用 `std::stod` 并捕获异常（防御性；CLI11 已保证 Range 校验）。
*   保留 `step.cli_params` 作为原始参数记录（元数据），不删除。

### 2.2 缺陷 3 修复方案

| 位置 | 现状 | 修复 |
| :--- | :--- | :--- |
| `pipeline_runner.cpp:444` | 硬编码 `real_esrgan_x4_plus`（models_info 中不存在） | 改为 `real_esrgan_x4` |
| `ParamMeta::default_value`（registry.ixx + pipeline_adapters.cpp 4 处 + 测试 3 处） | 死数据 | 删除字段及所有引用 |
| `docs/user/{zh,en}/cli_reference.md` 默认值列 | face_enhancer=codeformer、frame_enhancer=real_esrgan_x4 | 改为 app_config 实际默认：face_enhancer=gfpgan_1.4、frame_enhancer=real_esrgan_x2_fp16，并注明默认值来源 |

## 3. 实施路线图

### 3.1 阶段一: CLI 处理器参数链路打通（缺陷 1）

**目标**: 快捷模式 `--face-swapper-model` 等参数真实生效。

*   [ ] **任务 1.1**: 编写失败测试 `ApplyCliParamsToStep`（config_merger_test.cpp 新增用例） → 对应 Task: [C++_task_apply_cli_params.md](./fix-cli-processor-params/task/C++_task_apply_cli_params.md)
*   [ ] **任务 1.2**: 实现 `ApplyCliParamsToStep` 并接入 `build_quick_task_config`
*   [ ] **验收标准**:
    *   🔴 TDD 合规: 失败测试先行编写
    *   编译通过（无警告）
    *   🧪 单元测试全部通过

### 3.2 阶段二: e2e 脚本与默认模型名修复（缺陷 2 + 3）

**目标**: e2e 测试可运行；默认模型名全局一致。

*   [ ] **任务 2.1**: 修复 `test_cli_flags.py` 删除 `--headless`（CLI 程序无 GUI，该标志无意义） → 对应 Task: [C++_task_e2e_headless.md](./fix-cli-processor-params/task/C++_task_e2e_headless.md)
*   [ ] **任务 2.2**: 修正 runner 硬编码 `real_esrgan_x4_plus` → `real_esrgan_x4`；删除 `ParamMeta::default_value` 死数据（含测试同步更新） → 对应 Task: [C++_task_default_models.md](./fix-cli-processor-params/task/C++_task_default_models.md)
*   [ ] **验收标准**:
    *   🧪 单元测试全部通过（含修改后的 registry 测试）
    *   静态检查无遗留引用

### 3.3 阶段三: 集成验证与文档同步

**目标**: 全量测试通过，文档与实现一致。

*   [ ] **任务 3.1**: 集成测试 `python build.py --action test --test-label integration`
*   [ ] **任务 3.2**: 更新 [cli_reference.md](../../user/zh/cli_reference.md) 与 [cli_reference.md](../../user/en/cli_reference.md) 默认值列（含文档控制信息版本号）
*   [ ] **验收标准**:
    *   🧪 完整测试套件通过
    *   中英文文档同步更新

## 4. 风险管理

| 风险点 | 可能性 | 影响 | 缓解措施 |
| :--- | :--- | :--- | :--- |
| `step.cli_params` 删除后遗留引用 | 低 | 编译失败 | grep 全量验证后删除 |
| stod 解析失败导致崩溃 | 低 | 运行异常 | 防御性 try/catch 返回 ConfigError |
| CLI 层调用 config 层函数破坏分层 | 低 | 架构违规 | 函数放 config.merger，CLI 仅调用（App→Service 方向合法） |

## 5. 资源与依赖
*   **外部依赖**: 无新增
*   **前置任务**: 无
*   **关键工具链**: CMake + Ninja、GCC/Clang C++20、`python build.py`、Google Test

## 6. 全局研究发现记录 (Research Findings)

> 状态：全部任务已完成，本计划文档归档。

| 时间 | 发现内容 | 来源 | 影响评估 | 相关阶段 |
| :--- | :--- | :--- | :--- | :---: |
| 2026-08-14 15:30 | `cli_params` 全项目仅 4 处引用（定义+写入+注册），无消费点 | grep 全量检索 | 确认缺陷 1 根因 | 1.x |
| 2026-08-14 15:30 | `parse_face_selector_mode` 为 config 层公开函数，可直接复用 | config_parser.cpp:245 | 转换逻辑可复用现有解析 | 1.x |
| 2026-08-14 15:30 | `default_value` 被 processor_param_registry_test.cpp:41 引用 | 单元测试 | 删除需同步更新 3 处测试 | 2.x |
| 2026-08-14 15:30 | `real_esrgan_x4_plus` 在 models_info.json 中不存在 | models_info.json | runner 兜底值是死值 | 2.x |
| 2026-08-14 15:45 | config.parser 库依赖 config_core（parser→core），merger 若 import parser 会形成物理库循环依赖 | CMakeLists.txt | 枚举转换在 merger 内本地实现 | 1.x |
| 2026-08-14 15:45 | Red 确认：5 个 ApplyCliParamsToStep 测试失败，其余全通过；EmptyNoOp 用例在占位实现下即通过（符合预期） | ctest 输出 | 测试有效 | 1.x |
| 2026-08-14 15:45 | shutdown_handler_test 偶发 Bus error 为并行 gtest_discover 阶段环境问题，重试后通过 | ctest 输出 | 与本修复无关 | 1.x |
