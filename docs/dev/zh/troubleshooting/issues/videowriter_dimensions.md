# Issue: VideoWriter 帧尺寸无效 (Invalid Frame Dimensions)

## 问题描述
流水线在处理过程中报错：`VideoWriter: Invalid frame dimensions`，导致视频输出失败。

## 根因分析
- **过早初始化**: `PipelineRunner.cpp` 在收到第一帧经过 AI 增强（如上采样 4x）的帧之前，就尝试用原始输入视频的尺寸打开 `VideoWriter`。
- **尺寸不匹配**: 当 4K 的增强帧到达已锁定为 720p 尺寸的 Writer 时，底层 FFmpeg 适配器因尺寸不匹配抛出错误。

## 解决方案
- **延迟开启 (Deferred Open)**: 修改 `PipelineRunner` 逻辑，在第一帧有效结果产生后再根据结果帧的 `width`/`height` 动态打开 Writer。

## 相关链接
- [Pipeline 内部逻辑](../pipeline_internals.md) (待更新)
