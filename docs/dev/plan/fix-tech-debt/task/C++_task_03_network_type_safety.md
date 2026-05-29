     1|# 子任务 3: network.ixx 类型安全
     2|
     3|## 基本信息
     4|- **所属计划**: fix-tech-debt
     5|- **优先级**: P1
     6|- **修改文件**: `src/foundation/infrastructure/network.ixx`, `src/foundation/infrastructure/network.cpp`
     7|- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e
     8|
     9|## 目标
    10|将 `network.ixx` 和 `network.cpp` 中的 `long` 替换为 `std::int64_t`，确保跨平台类型宽度一致。
    11|
    12|## 具体改动
    13|1. **network.ixx** (3 处)
    14|   - 第 46 行: `long get_file_size_from_url(...)` → `std::int64_t get_file_size_from_url(...)`
    15|   - 第 60 行: `std::string human_readable_size(long size)` → `std::string human_readable_size(std::int64_t size)`
    16|2. **network.cpp** (6 处)
    17|   - 第 18 行: 声明同步
    18|   - 第 116/176 行: `long http_code` → `std::int64_t http_code`
    19|   - 第 155/221 行: 函数签名同步
    20|   - 第 198/201 行: `long remote_size` / `long local_size` → `std::int64_t`
    21|3. **确保 `#include <cstdint>` 存在**
    22|
    23|## 测试策略
    24|- 编译验证
    25|- 现有 network 相关测试通过
    26|
    27|## 验收标准
    28|- [ ] 编译通过
    29|- [ ] 无 `long` 类型残留（在 network 模块中）
    30|- [ ] 现有测试通过
    31|