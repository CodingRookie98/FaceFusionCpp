# C++ Task: 默认模型名统一与注册表死数据清理（缺陷 3）

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-TASK-CLI-PARAMS-03-2026
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

消除默认模型名三处不一致与注册表 `default_value` 死数据。

## 背景

| 位置 | 现状 | 问题 |
| :--- | :--- | :--- |
| `pipeline_runner.cpp:444` | 硬编码 `real_esrgan_x4_plus` | 模型名在 models_info.json 中不存在 |
| `ParamMeta::default_value` | 注册表字段，从未被消费 | 死数据，误导文档 |
| `cli_reference.md` 默认值列 | face_enhancer=codeformer、frame_enhancer=real_esrgan_x4 | 与实际（app_config 默认）不符 |

## 实现

1. **runner 兜底修正**: `pipeline_runner.cpp:444` `real_esrgan_x4_plus` → `real_esrgan_x4`。
2. **删除死数据**:
   - `processor_param_registry.ixx`: 删除 `ParamMeta::default_value` 字段；
   - `pipeline_adapters.cpp`: 4 处注册删除 default_value 实参；
   - `tests/unit/domain/pipeline/processor_param_registry_test.cpp`: 同步删除 3 处引用（41 行断言、76-80 行注册、97 行注册）；
   - grep 验证无遗留引用。
3. **文档修正**（阶段三执行）: `cli_reference.md`（zh/en）默认值列改为 app_config 实际默认（face_enhancer=gfpgan_1.4、frame_enhancer=real_esrgan_x2_fp16），并注明"默认值来自 app_config.yaml 的 default_models，可通过配置修改"。

## 验收

- [ ] 单元测试通过（含修改后的 registry 测试）
- [ ] grep `default_value` 无 src/tests 引用
- [ ] grep `real_esrgan_x4_plus` 无 src 引用

## 完成记录

- 完成时间: 2026-08-14
- 完成方式: runner 硬编码修正为 real_esrgan_x4；ParamMeta::default_value 字段及 13 处引用全部删除（registry.ixx + pipeline_adapters.cpp×9 + 测试×3）；grep 零残留；cli_reference.md 中英文默认值列已同步修正
- Commit ID: 待提交
