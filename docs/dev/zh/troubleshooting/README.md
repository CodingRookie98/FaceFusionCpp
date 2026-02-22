# 故障排查知识库 (Troubleshooting Knowledge Base)

本知识库收集了开发过程中遇到的常见问题、根因分析及解决方案。

## 1. 问题分类

为了便于检索，问题按照以下分类进行组织：

- **环境与构建 (Build)**: 编译器版本、库缺失、vcpkg 下载失败等。
- **内存与资源 (Memory)**: 显存溢出 (OOM)、背压失效、资源泄漏。
- **CUDA/TensorRT (Device)**: 驱动不兼容、算子不支持、引擎加载崩溃。
- **业务逻辑 (Logic)**: 人脸检测失败、视频同步问题、Pipeline 挂起。

## 2. 问题索引 (Issues List)

| 类别 | 问题摘要 | 状态 | 详细链接 |
| :--- | :--- | :--- | :--- |
| Build | C++20 Modules 不被支持 | 已解决 | [Issue #1](./issues/cpp20_modules.md) |
| Device | TensorRT Myelin 进程退出崩溃 | 已解决 | [Issue #2](./issues/trt_myelin_crash.md) |
| Memory | 视频测试超时 (严格内存模式) | 已分析 | [Issue #3](./issues/video_timeout.md) |
| Logic | VideoWriter 帧尺寸无效 | 已修复 | [Issue #4](./issues/videowriter_dimensions.md) |

## 3. 贡献规范

如果你发现了一个新问题并解决了它，请按照 [维护指南](../quickstart.md) 将其记录在 `issues/` 目录下，并在此 README 中添加索引。
