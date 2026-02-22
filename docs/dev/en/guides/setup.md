# Setup & Build Guide

This guide is intended to help developers set up their development environment and successfully compile FaceFusionCpp on Windows and Linux.

## 1. Software Prerequisites

### Common Dependencies
- **CMake**: >= 3.28 (C++20 module support)
- **Python**: >= 3.10 (for running `build.py`)
- **Git**: For source control.
- **CUDA Toolkit**: >= 12.2
- **cuDNN**: >= 9.2
- **TensorRT**: >= 10.2

### Windows
- **Visual Studio 2022**: Version 17.10+ (must include "Desktop development with C++" workload).
- **Ninja**: Recommended as generator.

### Linux (Ubuntu 22.04+)
- **GCC**: >= 13 (C++20 module support).
- **FFmpeg Libraries**: Suggest installing base libraries via system package manager.

## 2. Quick Start

### 2.1 Clone and Initialize
```bash
git clone https://github.com/CodingRookie98/faceFusionCpp.git
cd faceFusionCpp
pip install colorama  # build.py dependency
```

### 2.2 Building with `build.py`
We provide a unified Python wrapper script to abstract away cross-platform CMake command differences.

#### Configuration Phase (Configure)
```bash
python build.py --action configure --preset debug
```
*This will automatically download vcpkg dependencies (e.g., OpenCV, ONNX Runtime). The first run takes some time.*

#### Compilation Phase (Build)
```bash
python build.py --action build
```

#### Running Tests (Test)
```bash
python build.py --action test
```

## 3. CMake Presets

The project uses `CMakePresets.json` to manage build configurations:
- `msvc-x64-debug`: Windows Debug version.
- `msvc-x64-release`: Windows Release version (optimized).
- `linux-x64-debug`: Linux Debug version.

## 4. Troubleshooting

If you encounter "C++20 modules not supported," please check:
1. VS version is >= 17.10 on Windows.
2. GCC version is >= 13 on Linux.
3. CMake version is >= 3.28.
