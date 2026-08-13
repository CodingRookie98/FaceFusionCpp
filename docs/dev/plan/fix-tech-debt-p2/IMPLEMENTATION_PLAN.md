# 实施计划：修复技术债务与代码坏味道 - 阶段二 P2 架构重构

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-TD2-2026
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


## 概览

阶段二专注于架构层面的重构，需要更深入的测试覆盖验证。包含 2 个子任务。

## 阶段二：P2 架构重构（需测试覆盖验证）

### 子任务 7: shutdown_handler.ixx PIMPL 封装
**目标**: 将平台相关实现和私有成员隐藏到 .cpp
**修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/services/pipeline/shutdown_handler.cpp`
**具体改动**:
- 移除 `.ixx` 中的 `#include <Windows.h>` 和 `#ifdef _WIN32`
- 将所有 `private` 成员移入 `.cpp` 的 PIMPL 或匿名命名空间
- `.ixx` 仅保留 public 接口 + 前向声明
**测试**: shutdown_handler 相关测试 + 编译验证
**状态**: 未开始

### 子任务 8: FFmpeg RAII 封装
**目标**: 用 unique_ptr+custom deleter 替代手动 cleanup()
**修改文件**: `src/foundation/media/ffmpeg_reader.cpp`, `src/foundation/media/ffmpeg_writer.cpp`
**具体改动**:
- 为 AVFormatContext, AVCodecContext, SwsContext 等定义 RAII wrapper 类型别名
- 替换 28 处手动 `cleanup()` 调用
- 确保析构路径安全
**测试**: ffmpeg_reader/ffmpeg_writer 相关测试 + 集成测试
**状态**: 未开始

## 测试策略

### 单元测试
- `shutdown_handler_test.cpp` - 验证 PIMPL 封装后功能不变
- `ffmpeg_test.cpp` - 验证 RAII 资源管理正确

### 集成测试
- `component_integration_tests` - 验证跨模块协作
- `app_pipeline_tests` - 验证完整业务流程

## 风险评估

### 子任务 7 风险
- **平台兼容性**：需要确保 Linux 和 Windows 都能编译通过
- **信号处理**：POSIX 信号处理可能需要特殊处理

### 子任务 8 风险
- **资源生命周期**：某些 FFmpeg 资源可能需要延迟释放
- **异常路径**：需要确保所有异常路径都能正确释放资源

## 预计工作量
- 子任务 7: 2-3 小时
- 子任务 8: 3-4 小时
- 测试验证: 1-2 小时
- **总计**: 6-9 小时
