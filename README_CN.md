<div align="center">

# FaceFusionCpp

[![CI](https://github.com/CodingRookie98/faceFusionCpp/actions/workflows/ci.yml/badge.svg)](https://github.com/CodingRookie98/faceFusionCpp/actions/workflows/ci.yml)
[![License: GPLv3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C.svg)](https://en.cppreference.com/w/cpp/20)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()

> **FaceFusion 的高性能生产级 C++ 实现**
>
> 🚀 *更快、更轻、告别环境配置地狱！*

[[English Documentation]](README.md) | [[下载发布版]](https://github.com/CodingRookie98/faceFusionCpp/releases) | [[文档中心]](docs/user/zh/getting_started.md)

</div>

## 🌟 项目简介

**FaceFusionCpp** 是知名开源项目 [facefusion](https://github.com/facefusion/facefusion) 的高性能 C++ 重构版本。我们旨在为您提供完全一致且强大的面部处理能力，同时带来成倍提升的性能表现、极低的硬件资源占用，以及极致丝滑的免配置部署体验。

## 🏆 核心优势 (为什么选择 FaceFusionCpp？)

- 🚀 **极致运行性能**：深度优化的 C++ 核芯引擎，全面拥抱多线程并发架构，压榨硬件极限，吞吐量远超 Python 版本。
- 🧠 **原生硬件加速**：剥离 Python 冗余包装，原生调用 **NVIDIA CUDA** 与 **TensorRT** API，带给你闪电般的推理速度体验。
- 📦 **告别环境配置地狱**：无需繁琐的 Python 依赖安装、无需配置虚拟环境、不再受依赖冲突折磨。下载预编译的二进制文件，开箱即用（Out-of-the-Box）。
- 💾 **超低显存与内存占用**：通过精细的 C++ 内存管理，大幅度降低运行时 RAM 与 VRAM 消耗，让普通消费级显卡也能流畅运行大批量、高分辨率任务。
- 🌍 **全平台完美支持**：将 **Windows x64** 和 **Linux x64** 作为第一公民，提供一致且出色的稳定体验。

## ✨ 核心特性

- 🎭 **人脸替换 (Face Swapping)**：以极高的保真度与结构一致性，自动替换图像或视频中的面部。
- ✨ **人脸增强 (Face Enhancement)**：搭载先进的图像复原模型，对模糊面部进行修复、锐化和高清化重构。
- 😊 **表情复原 (Expression Restoration)**：精准还原源视频/源图像的表情细节，拒绝僵硬，真实自然。
- 🔍 **超分辨率 (Super Resolution)**：无损倍增低分辨率图像/视频的画质，消除噪点与伪像。
- ⚡ **批量处理引擎**：可并行调度图像与视频流的混合型批量计算，大幅节约创作时间。

## 📥 安装指南

体验 FaceFusionCpp 最简单的方法是前往我们的 [Releases 页面](https://github.com/CodingRookie98/faceFusionCpp/releases) 下载最新版本的预编译包。

### 系统要求

为确保引擎的最佳效能及平稳运行，请确认您的设备环境满足以下标准：

| 组件 | 最低要求 | 测试版本 | 说明 |
|-----------|-------------|----------------|-------|
| **操作系统** | Windows / Linux | Win10 / Ubuntu 24.04 | 仅支持 x64 架构 |
| **CUDA** | ≥ 12 | 12.8.1 | GPU 加速必需项 |
| **cuDNN** | ≥ 9 | 9.19.0 | GPU 加速必需项 |
| **TensorRT**| ≥ 10 | 10.15.1 | 可选推荐配置，开启后性能逆天 |

## 🚀 快速开始

解压下载的压缩包后，在所在目录打开终端或命令提示符（Terminal / CMD），执行：

**方式一：命令行参数**
```bash
# 命令行运行示例代码
./facefusion_cpp --source source.jpg --target target.mp4 --output result.mp4
```

**方式二：任务配置文件（推荐用于复杂工作流）**
```bash
# 使用 YAML 配置文件的运行示例代码
./facefusion_cpp --task_config config/task_config.yaml
```

*如需根据您的系统从源码编译，请参阅[源码构建指南](docs/dev/zh/guides/setup.md)。*

## 📖 文档中心

查阅我们详实精美的官方文档，掌握 FaceFusionCpp 的全部潜能：

- **[快速开始指南](docs/user/zh/getting_started.md)**：包含安装说明、依赖检查与入门 Demo。
- **[完整用户手册](docs/user/zh/user_guide.md)**：深入剖析各类功能模块的组合使用技巧。
- **[CLI 参数参考](docs/user/zh/cli_reference.md)**：命令行参数的详细检索字典。
- **[核心配置指南](docs/user/zh/configuration_guide.md)**：教你玩转 `app_config.yaml` 和 `task_config.yaml` 进阶设定。
- **[硬件性能指南](docs/user/zh/hardware_guide.md)**：不同硬件的兼容与算力最大化兵法。
- **[常见问题 (FAQ)](docs/user/zh/faq.md)**：遇到报错怎么办？来这里寻找答案。
- **[开发者中心](docs/dev/zh/architecture/design.md)**：如果你想参与贡献代码或了解架构，此处必读。

## 🙏 鸣谢

特别感谢原创 Python 开源项目 [facefusion](https://github.com/facefusion/facefusion) 各位作者和贡献者在开源社区做出的先驱性探索和无私奉献。FaceFusionCpp 正是站在巨人的肩膀上才得以诞生。

## ⚖️ 免责声明

本软件及相关衍生品仅供且严格局限于**教育、科学与技术研究目的**使用。任何下载、使用本软件的自然人或法人对其行为承担全部及唯一责任。
严禁将本软件用于任何形式的非法用途、严禁在未经当事人许可情况下制作深度伪造（Deepfake）视频、严禁制作任何侵犯他人隐私权、肖像权或合法权益的内容。

FaceFusionCpp 开发团队及相关贡献者不对任何人因不当使用本工具而造成的任何后果及纠纷承担责任。

## 📄 许可证

本项目开源采用 **GPLv3 许可证** 授权 - 详情请参阅项目根目录下的 [LICENSE](LICENSE) 文件。

---

<p align="center">
  <sub>Built with ❤️ by the FaceFusionCpp Team. For the Community.</sub>
</p>
