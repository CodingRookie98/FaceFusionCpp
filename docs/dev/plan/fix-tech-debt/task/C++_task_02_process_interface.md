# C++ task 02 process interface

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-TD1-T02-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |

     1|# 子任务 2: process.ixx 接口修正
     2|
     3|## 基本信息
     4|- **所属计划**: fix-tech-debt
     5|- **优先级**: P1
     6|- **修改文件**: `src/foundation/infrastructure/process.ixx`
     7|- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e
     8|
     9|## 目标
    10|为 `Process` 构造函数添加 `explicit`，为查询函数添加 `[[nodiscard]]`。
    11|
    12|## 具体改动
    13|1. **explicit 构造函数** (第 75 行、第 89 行)
    14|   - `Process(const string_type& command, ...)` → `explicit Process(const string_type& command, ...)`
    15|   - `Process(const std::vector<string_type>& arguments, ...)` → `explicit Process(const std::vector<string_type>& arguments, ...)`
    16|   - 理由：虽有 6 个参数，但后 4 个有默认值，`Process p = "cmd"` 可触发隐式转换
    17|2. **[[nodiscard]] 查询函数** (第 100/106/113 行)
    18|   - `id_type get_id()` → `[[nodiscard]] id_type get_id()`
    19|   - `int get_exit_status()` → `[[nodiscard]] int get_exit_status()`
    20|   - `bool try_get_exit_status(int&)` → `[[nodiscard]] bool try_get_exit_status(int&)`
    21|
    22|## 测试策略
    23|- 编译验证
    24|- 现有 process 相关测试全部通过
    25|
    26|## 验收标准
    27|- [ ] 编译通过
    28|- [ ] 现有测试通过
    29|- [ ] 无隐式转换可能
    30|