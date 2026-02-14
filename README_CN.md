# FaceFusionCpp

> **FaceFusion 的高性能 C++ 实现**
>
> 🚀 *更快、更轻、生产级就绪*

[[English Documentation]](https://github.com/CodingRookie98/faceFusionCpp/blob/master/README.md)

FaceFusionCpp 是知名开源项目 [facefusion](https://github.com/facefusion/facefusion) 的高性能 C++ 重写版本。旨在提供同样强大的面部处理能力，同时显著提升性能、降低资源占用，并简化部署流程。

## ✨ 核心特性

- **⚡ 极致性能**：深度优化的 C++ 核心，支持多线程并行处理，吞吐量最大化。
- **🚀 硬件加速**：原生集成 CUDA 和 TensorRT，提供闪电般的推理速度。
- **🛠 全面工具集**：
  - **人脸替换**：高保真地替换图片和视频中的人脸。
  - **人脸增强**：修复和锐化面部细节，提升清晰度。
  - **表情复原**：还原原始面部表情，更加自然。
  - **超分辨率**：提升低分辨率图片和视频的画质。
- **📦 批量处理**：高效处理图片和视频混合输入。
- **🔧 跨平台支持**：完美支持 **Windows x64** 和 **Linux x64**。

## 📥 安装指南

请访问 [Releases 页面](https://github.com/CodingRookie98/faceFusionCpp/releases) 下载最新版本的预编译程序。

### 系统要求

为确保最佳性能和兼容性，请确保您的系统满足以下要求：

| 组件 | 最低要求 | 测试版本 | 说明 |
|-----------|-------------|----------------|-------|
| **操作系统** | Windows / Linux | Win10 / Ubuntu 24.04 | 仅支持 x64 架构 |
| **CUDA** | ≥ 12.2 | 12.8.1 | GPU 加速必需 |
| **cuDNN** | ≥ 9.2 | 9.19.0 | GPU 加速必需 |
| **TensorRT**| ≥ 10.2 | 10.15.1 | 可选，开启以获得极致性能 |

## 📖 文档中心

查阅我们的详细文档以快速上手：

- **[快速开始](docs/user/zh/getting_started.md)**：安装教程与首次运行。
- **[用户指南](docs/user/zh/user_guide.md)**：详细的功能使用说明。
- **[CLI 参考](docs/user/zh/cli_reference.md)**：完整的命令行参数手册。
- **[配置指南](docs/user/zh/configuration_guide.md)**：高级配置选项说明。
- **[硬件指南](docs/user/zh/hardware_guide.md)**：硬件兼容性与配置建议。
- **[常见问题 (FAQ)](docs/user/zh/faq.md)**：常见问题解答。

## ⚖️ 免责声明

本软件仅供**教育和研究目的**使用。用户有责任确保其使用行为符合当地法律法规。严禁将本软件用于非法目的或制作侵犯他人权益的内容。

FaceFusionCpp 的开发者不对任何滥用本软件的行为承担责任。

## 📄 许可证

本项目采用 MIT 许可证 - 详情请参阅 [LICENSE](LICENSE) 文件。

---

<p align="center">
  <sub>Built with ❤️ by the FaceFusionCpp Team</sub>
</p>
