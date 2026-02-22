# Issue: 视频测试超时 (Strict Memory Timeout)

## 问题描述
在集成测试中，`PipelineRunnerVideoTest.ProcessVideoStrictMemory` 在 120 秒内未能完成，触发超时。

## 根因分析
1. **构建模式**: C++ Debug 构建版本由于缺乏优化和额外的安全检查，运行速度极慢。
2. **内存策略**: `strict` 模式频繁读写磁盘临时文件以降低内存占用，进一步增加了 IO 开销。
3. **视频规格**: 测试视频 `slideshow_scaled.mp4` 片段在 5K 上采样（RealESRGAN x4）模式下，即使是几秒钟的视频也需要极长的编码时间。

## 解决方案
- **临时方案**: 在 `tests/integration/CMakeLists.txt` 中增加该测试项的超时时间（如 600s）。
- **长期方案**: 在集成测试中改用更小功率的模型（如 `real_esrgan_x2_fp16`）或大幅缩减测试视频的帧数。

## 相关链接
- [系统配置指南](../../user/configuration_guide.md#内存策略)
