# Build Guide

This guide describes how to build FaceFusionCpp from source on Windows and Linux.

## 1. Prerequisites

### Common Requirements
*   **CMake**: >= 3.28
*   **Python**: >= 3.10 (for build scripts)
*   **Ninja**: Recommended build system (required for C++20 Modules in some setups).
*   **Git**: For cloning the repository.
*   **CUDA Toolkit**: >= 12.2
*   **cuDNN**: >= 9.2
*   **TensorRT**: >= 10.2
*   **FFmpeg**: >= 7.0 (Libraries: avcodec, avformat, avutil, swscale)

### Windows
*   **Visual Studio 2022**: Version 17.10 or later (for C++20 Modules support).
*   **Desktop development with C++** workload installed.

### Linux (Ubuntu 22.04+)
*   **GCC**: >= 13 (must support C++20 modules).
*   **Dependencies**:
    ```bash
    sudo apt-get install build-essential cmake ninja-build python3 python3-pip git
    # FFmpeg libraries (if not using vcpkg or local build)
    sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev
    ```

---

## 2. Environment Setup

### 2.1 Clone Repository
```bash
git clone https://github.com/CodingRookie98/faceFusionCpp.git
cd faceFusionCpp
```

### 2.2 Install Python Dependencies
The build script (`build.py`) requires `colorama` for colored output.
```bash
pip install colorama
```

### 2.3 Setup CUDA/TensorRT
Ensure `CUDA_PATH` and `TENSORRT_ROOT` (or similar) environment variables are set correctly so CMake can find them.

---

## 3. Building with `build.py`

We provide a Python script to wrap CMake commands and handle cross-platform differences.

### 3.1 Configure
Generates build files.
```bash
python build.py --action configure
```
*   This uses the default `debug` preset (`msvc-x64-debug` on Windows, `linux-x64-debug` on Linux).
*   To configure for release: `python build.py --action configure --preset release`

### 3.2 Build
Compiles the source code.
```bash
python build.py --action build
```
*   **Note**: The first build will take time as it compiles vcpkg dependencies (OpenCV, ONNX Runtime, etc.).

### 3.3 Test
Runs the unit tests.
```bash
python build.py --action test
```

### 3.4 Install
Installs the executable and assets to the `install/` directory.
```bash
python build.py --action install
```

### 3.5 Package
Creates a zip archive for distribution.
```bash
python build.py --action package
```

---

## 4. CMake Presets

This project uses `CMakePresets.json` to define build configurations.

| Preset | OS | Type | Description |
| :--- | :--- | :--- | :--- |
| `msvc-x64-debug` | Windows | Debug | Default for dev. Optimizations off, debug symbols on. |
| `msvc-x64-release` | Windows | Release | Optimizations on, LTO enabled. |
| `linux-x64-debug` | Linux | Debug | Default for dev. |
| `linux-x64-release` | Linux | Release | Production build. |

---

## 5. Troubleshooting

### "C++20 modules not supported"
*   **Windows**: Update Visual Studio to 17.10+.
*   **Linux**: Ensure you are using GCC 13+ and CMake 3.28+.

### "Could not find TensorRT"
*   Check if TensorRT is installed.
*   Set `TENSORRT_ROOT` env var to your TensorRT installation path.

### "vcpkg install failed"
*   Check your internet connection (github.com access).
*   Check `build/.../vcpkg-manifest-install.log` for details.
