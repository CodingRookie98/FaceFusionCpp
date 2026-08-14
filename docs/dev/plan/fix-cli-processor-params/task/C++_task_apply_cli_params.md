# C++ Task: ApplyCliParamsToStep 实现（CLI 处理器参数链路打通）

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-TASK-CLI-PARAMS-01-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 依据实施计划创建。 |

## 目标

快捷模式下 CLI 处理器参数（`--face-swapper-model` 等 15 个标志）当前仅写入 `step.cli_params` 死胡同，需打通到 `step.params`（typed variant）使参数真实生效。

## 背景

- `build_quick_task_config`（app_cli.cpp:506-552）只写 `step.cli_params[param] = value`；
- Runner（pipeline_runner.cpp）与 `MergeConfigs`（config_merger.cpp）只消费 `step.params`；
- YAML 路径（config_parser.cpp ParsePipelineStep:489-551）已展示完整转换模式，`parse_face_selector_mode` 可复用。

## 🔴 Red: 失败测试（先行编写）

在 `tests/unit/app/config/config_merger_test.cpp` 新增测试用例（`ApplyCliParamsToStep`）：

1. `face_swapper` 步骤：`cli_params = {model=inswapper_128, face_selector_mode=reference, reference_face_path=ref.jpg}` → 断言 `params.model == "inswapper_128"`、`face_selector_mode == FaceSelectorMode::Reference`、`reference_face_path == "ref.jpg"`
2. `face_enhancer` 步骤：`blend_factor=0.9` → 断言 `blend_factor == 0.9`
3. `expression_restorer` 步骤：`restore_factor=0.5` → 断言生效
4. `frame_enhancer` 步骤：`model=real_esrgan_x8, enhance_factor=0.7` → 断言生效
5. 非法枚举 `face_selector_mode=xxx` → 断言返回错误（防御性）
6. 空 cli_params → 断言 params 保持不变（无副作用）

## 🟢 Green: 实现

1. `config_merger.ixx` 新增导出声明：
   ```cpp
   /// Apply explicit CLI parameter overrides to a pipeline step's typed params.
   /// Returns ConfigError on invalid enum/numeric values (defensive; CLI11 pre-validates).
   Result<void, ConfigError> ApplyCliParamsToStep(PipelineStep& step);
   ```
2. `config_merger.cpp` 实现：参照 ParsePipelineStep 模式，按 `step.step` 分发，读取 `step.cli_params`，用 `std::stod`（try/catch）转换数值、`parse_face_selector_mode` 转换枚举、直接赋值 string/path。
3. `app_cli.cpp` `build_quick_task_config`：填充 `cli_params` 后调用 `ApplyCliParamsToStep`，失败时记录日志（CLI11 已预校验，理论不可达）。

## 🔵 Refactor

- 转换逻辑保持单一职责；错误路径显式返回，不吞异常。

## 验收

- [ ] 单元测试通过：`python build.py --action test --test-label unit`
- [ ] 编译通过（无警告）
- [ ] 快捷模式 `--validate` + `--face-swapper-model inswapper_128` 可通过

## 完成记录

- 完成时间: 2026-08-14
- 完成方式: Red（5 个失败用例）→ Green（全部通过）；ApplyCliParamsToStep 实现于 config_merger.cpp（本地枚举转换避免 parser 物理库循环依赖）；build_quick_task_config 接线完成；单元测试全绿
- Commit ID: 待提交
