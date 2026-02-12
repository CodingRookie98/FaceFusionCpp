# 构建指南 (Build Guide)

本指南介绍了如何在 Windows 和 Linux 上从源码构建 FaceFusionCpp。

## 1. 前置条件 (Prerequisites)

### 通用要求
*   **CMake**: >= 3.28
*   **Python**: >= 3.10 (用于构建脚本)
*   **Ninja**: 推荐的构建系统 (某些设置下必须支持 C++20 模块)。
*   **Git**: 用于克隆仓库。
*   **CUDA Toolkit**: >= 12.2
*   **cuDNN**: >= 9.2
*   **TensorRT**: >= 10.2
*   **FFmpeg**: >= 7.0 (Libraries: avcodec, avformat, avutil, swscale)

### Windows
*   **Visual Studio 2022**: 版本 17.10 或更高 (用于支持 C++20 模块)。
*   **C++ 桌面开发**工作负载已安装。

### Linux (Ubuntu 22.04+)
*   **GCC**: >= 13 (必须支持 C++20 模块)。
*   **依赖项**:
    ```bash
    sudo apt-get install build-essential cmake ninja-build python3 python3-pip git
    # FFmpeg 库 (如果不使用 vcpkg 或本地构建)
    sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
    ```

---

## 2. 环境搭建 (Environment Setup)

### 2.1 克隆仓库
```bash
git clone https://github.com/CodingRookie98/faceFusionCpp.git
cd faceFusionCpp
```

### 2.2 安装 Python 依赖
构建脚本 (`build.py`) 需要 `colorama` 来显示彩色输出。
```bash
pip install colorama
```

### 2.3 设置 CUDA/TensorRT
确保 `CUDA_PATH` 和 `TENSORRT_ROOT` (或类似) 环境变量设置正确，以便 CMake 能够找到它们。

---

## 3. 使用 `build.py` 构建

我们提供了一个 Python 脚本来包装 CMake 命令并处理跨平台差异。

### 3.1 配置 (Configure)
生成构建文件。
```bash
python build.py --action configure
```
*   这使用默认的 `debug` 预设 (Windows 上为 `msvc-x64-debug`，Linux 上为 `linux-x64-debug`)。
*   要配置发布版本：`python build.py --action configure --preset release`

### 3.2 构建 (Build)
编译源代码。
```bash
python build.py --action build
```
*   **注意**: 第一次构建会花费较长时间，因为它会编译 vcpkg 依赖项 (OpenCV, ONNX Runtime 等)。

### 3.3 测试 (Test)
运行单元测试。
```bash
python build.py --action test
```

### 3.4 安装 (Install)
将可执行文件和资源安装到 `install/` 目录。
```bash
python build.py --action install
```

### 3.5 打包 (Package)
创建一个用于分发的 zip 压缩包。
```bash
python build.py --action package
```

---

## 4. CMake 预设 (CMake Presets)

本项目使用 `CMakePresets.json` 定义构建配置。

| 预设 | 操作系统 | 类型 | 描述 |
| :--- | :--- | :--- | :--- |
| `msvc-x64-debug` | Windows | Debug | 开发默认。关闭优化，开启调试符号。 |
| `msvc-x64-release` | Windows | Release | 开启优化，启用 LTO。 |
| `linux-x64-debug` | Linux | Debug | 开发默认。 |
| `linux-x64-release` | Linux | Release | 生产构建。 |

---

## 5. 故障排除 (Troubleshooting)

### "C++20 modules not supported"
*   **Windows**: 更新 Visual Studio 到 17.10+。
*   **Linux**: 确保您使用的是 GCC 13+ 和 CMake 3.28+。

### "Could not find TensorRT"
*   检查是否已安装 TensorRT。
*   将 `TENSORRT_ROOT` 环境变量设置为您的 TensorRT 安装路径。

### "vcpkg install failed"
*   检查您的网络连接 (github.com 访问)。
*   检查 `build/.../vcpkg-manifest-install.log` 获取详细信息。
