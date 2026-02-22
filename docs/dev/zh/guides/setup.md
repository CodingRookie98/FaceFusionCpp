# 环境搭建与构建指南 (Setup & Build Guide)

本指南旨在指导开发者如何在 Windows 和 Linux 环境下搭建开发环境并成功编译 FaceFusionCpp。

## 1. 前置软件要求

### 通用依赖
- **CMake**: >= 3.28 (支持 C++20 模块)
- **Python**: >= 3.10 (用于运行 `build.py`)
- **Git**: 用于源码管理。
- **CUDA Toolkit**: >= 12.2
- **cuDNN**: >= 9.2
- **TensorRT**: >= 10.2

### Windows
- **Visual Studio 2022**: 版本 17.10+ (必须包含 "C++ 桌面开发" 工作负载)。
- **Ninja**: 推荐作为生成器。

### Linux (Ubuntu 22.04+)
- **GCC**: >= 13 (支持 C++20 模块)。
- **FFmpeg 库**: 建议通过系统包管理器安装基础库。

## 2. 快速开始 (Quick Start)

### 2.1 克隆并初始化
```bash
git clone https://github.com/CodingRookie98/faceFusionCpp.git
cd faceFusionCpp
pip install colorama  # build.py 依赖
```

### 2.2 使用 `build.py` 构建
我们提供了一个统一的 Python 包装脚本，屏蔽了跨平台的 CMake 命令差异。

#### 配置阶段 (Configure)
```bash
python build.py --action configure --preset debug
```
*这会自动下载 vcpkg 依赖（如 OpenCV, ONNX Runtime），初次运行需时较长。*

#### 编译阶段 (Build)
```bash
python build.py --action build
```

#### 运行测试 (Test)
```bash
python build.py --action test
```

## 3. CMake 预设说明

项目使用 `CMakePresets.json` 管理构建配置：
- `msvc-x64-debug`: Windows 调试版。
- `msvc-x64-release`: Windows 发布版（启用优化）。
- `linux-x64-debug`: Linux 调试版。

## 4. 常见问题排查

如果遇到 "C++20 modules not supported"，请检查：
1. Windows 上 VS 版本是否 >= 17.10。
2. Linux 上 GCC 版本是否 >= 13。
3. CMake 版本是否 >= 3.28。
