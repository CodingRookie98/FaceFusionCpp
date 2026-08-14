# C++ Task: e2e 测试 --headless 修复（缺陷 2）

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-TASK-CLI-PARAMS-02-2026
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

修复 `tests/e2e/scripts/test_cli_flags.py` 中未实现的 `--headless` 标志导致的必然失败。

## 背景

- 该脚本第 44 行传入 `--headless`，但 `app_cli.cpp` 未注册该标志；
- 实测 CLI11 拒绝未知参数：`The following argument was not expected: --headless`；
- FaceFusionCpp CLI 本身无 GUI，headless 语义无意义，删除即可（不新增标志）。

## 实现

- 删除 `test_cli_flags.py` 第 44 行 `"--headless"`。
- 验证脚本其余部分（--system-check、快捷模式 -s -t -o）语法正确。

## 验收

- [ ] 脚本不再包含 `--headless`（grep 验证）
- [ ] e2e 脚本语法检查通过：`python3 -m py_compile tests/e2e/scripts/test_cli_flags.py`
- [ ] （如环境允许）运行 e2e 验证快捷模式输出文件生成

## 完成记录

- 完成时间: 2026-08-14
- 完成方式: 删除 test_cli_flags.py 第 44 行 `--headless`；`python3 -m py_compile` 通过；grep 验证无残留
- Commit ID: 待提交
