# C++ task 05 rule of five

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-TD1-T05-2026
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

     1|# 子任务 5: Rule of 5 补齐
     2|
     3|## 基本信息
     4|- **所属计划**: fix-tech-debt
     5|- **优先级**: P1
     6|- **修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/services/pipeline/checkpoint_manager.ixx`
     7|- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e
     8|
     9|## 目标
    10|为声明了析构函数或拷贝删除的类补齐移动语义声明。
    11|
    12|## 具体改动
    13|1. **shutdown_handler.ixx** (第 115-116 行后追加)
    14|   ```cpp
    15|   ShutdownHandler(ShutdownHandler&&) = delete;
    16|   ShutdownHandler& operator=(ShutdownHandler&&) = delete;
    17|   ```
    18|2. **checkpoint_manager.ixx** (第 71-72 行后追加)
    19|   ```cpp
    20|   CheckpointManager(CheckpointManager&&) = delete;
    21|   CheckpointManager& operator=(CheckpointManager&&) = delete;
    22|   ```
    23|
    24|## 测试策略
    25|- 编译验证（声明为 delete 不影响运行时行为）
    26|- 现有 shutdown_handler 和 checkpoint_manager 测试通过
    27|
    28|## 验收标准
    29|- [ ] 编译通过
    30|- [ ] 现有测试通过
    31|- [ ] 所有特殊成员函数显式声明
    32|