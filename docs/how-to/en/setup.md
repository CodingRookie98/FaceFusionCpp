# Setup & Build Guide

> **Document Control**
> - **Document ID**: FFC-DEV-EN-GUIDE-SETUP-2026
> - **Version**: V1.1.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-17

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-17 | AI Agent | 王辉 | Added Web Development section: `build.py --action dev`, `FFC_WEB_PORT`/`FFC_WEB_HOST`, WebSocket auto-reconnect. |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | Initialized document control info per documentation governance. |


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

#### Web Development (dev)
One command starts both the C++ web backend (`ffc --web`) and the Vite
dev server with hot reload, keeps Vite's proxy in sync with the backend
port, pre-checks port conflicts, and cleans up the backend on exit:

```bash
python build.py --action dev [--web-port 8000]
```

The Vite proxy target is configurable via `FFC_WEB_PORT` (default `8000`)
and `FFC_WEB_HOST` (default `127.0.0.1`) environment variables; keep them
consistent with `ffc --web --web-port <port>`. The frontend WebSocket
client auto-reconnects with exponential backoff (1s -> 30s cap), so a
backend restart no longer requires a manual page refresh.

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
