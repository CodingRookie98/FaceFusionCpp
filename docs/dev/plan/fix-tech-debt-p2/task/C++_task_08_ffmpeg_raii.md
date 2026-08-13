# 子任务 8: FFmpeg RAII 封装

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-TD2-T08-2026
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


## 基本信息
- **所属计划**: fix-tech-debt-p2
- **优先级**: P2
- **修改文件**: `src/foundation/media/ffmpeg_raii.h`, `src/foundation/media/ffmpeg_raii.cpp`
- **状态**: 部分完成
- **完成时间**: 2026-05-29
- **Commit ID**: a2a2f53

## 目标
用 `unique_ptr` + 自定义 deleter 替代手动 `cleanup()` 调用，实现 RAII 资源管理。

## 已完成的工作

### 1. 创建 RAII 包装器基础框架
- 定义了 `AVFormatContextPtr`, `AVCodecContextPtr`, `AVFramePtr`, `AVPacketPtr`, `SwsContextPtr` 类型
- 实现了自定义 Deleter 和工厂函数
- 文件：`ffmpeg_raii.h`, `ffmpeg_raii.cpp`

### 2. 技术难点
- FFmpeg 的 `avformat_open_input` 需要二级指针，与 `unique_ptr` 不直接兼容
- 解码线程与帧队列的生命周期管理复杂
- 需要保持现有的异步解码架构

## 待完成的工作

### 1. 迁移 ffmpeg_reader.cpp
- 将 `Impl` 类的成员变量改为 RAII 类型
- 移除手动 `cleanup()` 调用
- 确保解码线程正确工作

### 2. 迁移 ffmpeg_writer.cpp
- 类似地将成员变量改为 RAII 类型
- 移除手动 `cleanup()` 调用

## 经验教训

1. **FFmpeg API 兼容性**：某些 FFmpeg API（如 `avformat_open_input`）需要二级指针，需要特殊处理
2. **异步架构复杂性**：解码线程与帧队列的生命周期管理需要仔细设计
3. **渐进式重构**：对于复杂的重构，应该分步进行，先验证核心逻辑

## 下一步建议

1. 在 `ffmpeg_reader.cpp` 中逐步引入 RAII 包装器
2. 先修改简单的资源（如 `AVFrame`, `AVPacket`），再修改复杂的资源（如 `AVFormatContext`）
3. 确保每一步都通过测试验证
