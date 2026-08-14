# 二进制重命名 ffc 实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-RENAME-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 依据用户决策（方案 A：完全重命名）创建。 |

## 1. 概述

将二进制产物 `FaceFusionCpp`/`FaceFusionCpp.exe` 重命名为 `ffc`/`ffc.exe`。

**核心原则：命令引用替换，产品名保留**
- `命令引用`（`./FaceFusionCpp`、`bin/FaceFusionCpp`、`FaceFusionCpp.exe`）→ 替换为 `ffc`
- `产品名`（"FaceFusionCpp 是一个…"、vcpkg name、project.yaml name、仓库名、DISCLAIMER）→ 保留
- 品牌关联：banner 与 `--help` 显示 `ffc (FaceFusionCpp)`

## 2. 影响面（已调研）

| 区域 | 文件 | 处理 |
| :--- | :--- | :--- |
| CMakeLists.txt | project() 名 + target 输出名 | project(FaceFusionCpp→ffc)；`set(app_facefusioncpp "ffc")`（变量名保留，减 diff） |
| src/app/version.cpp.in | `@PROJECT_NAME@` 注入 | 自动变为 ffc；banner 增加品牌行 (FaceFusionCpp) |
| src/app/cli/app_cli.cpp | CLI11 app 描述 | "ffc (FaceFusionCpp) - Face processing pipeline" |
| tests/e2e/scripts | run_e2e.py:61-62、test_batch_processing.py:12-13 glob | 探测 `bin/ffc` |
| docs/ | 10 个文件含命令引用 + 15 个仅产品名 | 命令引用替换；产品名保留 |
| README/README_CN | 命令示例 | 替换命令引用 |
| 保留项 | vcpkg.json name、project.yaml name、Ort::Env、assets/models_info.json、历史计划文档 | 不动 |

## 3. 任务

1. C++ 侧重命名（CMake + version + cli 描述）
2. e2e 脚本探测更新
3. 文档命令引用替换（docs + README）
4. 构建验证：产物 `build/bin/linux-x64-debug/ffc`、`ffc --version`、单元测试、e2e 探测
5. 提交并合并回 dev

## 4. 验收标准

- [ ] 构建产物为 `ffc`，`ffc --version` 输出 `ffc v0.34.1 (...)`
- [ ] 单元测试全部通过
- [ ] grep 验证：命令引用无 `FaceFusionCpp` 残留（仅产品名/历史计划文档允许）
- [ ] e2e run_e2e.py 自动探测成功

> 本计划为机械性重命名，无新逻辑代码，不适用 TDD 循环（验证靠构建 + 测试 + grep）。
