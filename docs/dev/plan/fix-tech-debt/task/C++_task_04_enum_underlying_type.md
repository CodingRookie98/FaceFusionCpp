     1|# 子任务 4: 剩余枚举底层类型补全
     2|
     3|## 基本信息
     4|- **所属计划**: fix-tech-debt
     5|- **优先级**: P1
     6|- **修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/app/cli/system_check.ixx`, `src/foundation/infrastructure/process.ixx`
     7|- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e
     8|
     9|## 目标
    10|为 3 个未指定底层类型的 `enum class` 添加 `: std::uint8_t`。
    11|
    12|## 具体改动
    13|1. **shutdown_handler.ixx** 第 31 行
    14|   - `enum class ShutdownState {` → `enum class ShutdownState : std::uint8_t {`
    15|2. **system_check.ixx** 第 10 行
    16|   - `enum class CheckStatus {` → `enum class CheckStatus : std::uint8_t {`
    17|3. **process.ixx** 第 31 行
    18|   - `enum class ShowWindow {` → `enum class ShowWindow : std::uint8_t {`
    19|   - 注意：`ShowWindow` 有 `force_minimize = 11`，uint8_t 范围 0-255 足够
    20|
    21|## 测试策略
    22|- 编译验证
    23|- 现有相关测试通过
    24|
    25|## 验收标准
    26|- [ ] 编译通过
    27|- [ ] 现有测试通过
    28|- [ ] 所有 3 个枚举均有显式底层类型
    29|