# 快速开始 (Getting Started)

欢迎使用 **FaceFusionCpp**，这是热门开源项目 [facefusion](https://github.com/facefusion/facefusion) 的高性能 C++ 实现。本文档将指导您完成环境准备、安装和首次人脸替换操作。

## 1. 系统要求

在下载之前，请确保您的系统满足以下要求。FaceFusionCpp 依赖 NVIDIA CUDA 进行 GPU 加速。

### 操作系统
*   **Windows 10/11 x64** (目前主要支持平台)
*   *Linux 支持目前主要用于开发和源码构建。*

### 硬件
*   **NVIDIA GPU**: 必需，用于高性能推理。
*   **VRAM**: 最低 4GB，建议 8GB+ 以处理 1080p+ 视频。

### 软件依赖
您必须安装以下组件并将其添加到系统 PATH 中，或将其放置在可执行文件同级目录：

| 组件 | 版本要求 | 测试版本 | 说明 |
| :--- | :--- | :--- | :--- |
| **CUDA Toolkit** | >= 12.2 | 12.5 | 必需，用于 GPU 执行。 |
| **cuDNN** | >= 9.2 | 9.2 | 必需，深度学习原语。 |
| **TensorRT** | >= 10.2 | 10.2 | 可选但推荐，用于最高速度。 |
| **FFmpeg** | >= 7.0.2 | 7.0.2 | 必需，用于视频处理。 |

> **注意**: 如果您下载的是预打包的 Release 版本，部分依赖（如 FFmpeg）可能已包含，或随附有放置 DLL 的说明。请确保 CUDA/cuDNN 已在您的系统中正确安装。

---

## 2. 安装步骤

1.  **下载 Release 包**:
    前往 [Releases](https://github.com/CodingRookie98/faceFusionCpp/releases) 页面下载适用于 Windows 的最新 `.zip` 包。

2.  **解压**:
    将压缩包解压到一个目录 (例如 `D:\FaceFusionCpp`)。
    *尽量避免路径中包含非 ASCII 字符或空格。*

3.  **目录结构**:
    解压后，您应该看到如下结构：
    ```
    FaceFusionCpp/
    ├── bin/                # 可执行文件和 DLL
    ├── models/             # AI 模型文件 (.onnx)
    ├── resources/          # 字体文件、遮罩等资源
    ├── FaceFusionCpp.exe   # 主程序
    └── ...
    ```

---

## 3. 首次运行 (人脸替换)

让我们执行一个基础的人脸替换操作来验证一切正常。

### 准备工作
1.  准备一张 **源图片 (Source Image)** (您想要使用的人脸)。假设命名为 `source.jpg`。
2.  准备一张 **目标图片 (Target Image)** (您想要被替换的图片)。假设命名为 `target.jpg`。
3.  为了方便，将它们放置在项目根目录。

### 执行
在安装目录打开终端 (PowerShell 或 CMD)，并运行：

```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg
```

**参数说明**:
*   `-s, --source`: 源人脸图片路径。
*   `-t, --target`: 目标图片或视频路径。
*   `-o, --output`: 结果保存路径。

### 预期输出
您应该看到日志显示：
1.  **设备初始化** (CUDA/TensorRT)。
2.  **模型加载** (inswapper_128 等)。
3.  **处理进度**。
4.  **完成信息**。

检查 `output.jpg` 即可查看结果！

---

## 4. 常见问题 (Troubleshooting)

如果程序启动失败或崩溃：

*   **"DLL not found"**: 确保 CUDA, cuDNN 和 TensorRT 的 `bin` 目录已添加到系统 PATH，或者将所需的 `.dll` 文件复制到 `FaceFusionCpp.exe` 旁边。
*   **"CUDA error"**: 将 NVIDIA 显卡驱动更新到最新版本。
*   **"Model not found"**: 确保 `models/` 目录存在并包含所需的 `.onnx` 文件。

有关更详细的配置选项，请参阅 [用户指南](user_guide.md) 或 [CLI 参考](cli_reference.md)。
