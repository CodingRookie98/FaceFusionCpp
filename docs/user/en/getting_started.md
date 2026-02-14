# Getting Started

Welcome to **FaceFusionCpp**, a high-performance C++ implementation of the popular [facefusion](https://github.com/facefusion/facefusion) project. This guide will help you set up your environment and perform your first face swap.

## 1. System Requirements

Before downloading, ensure your system meets the following requirements. FaceFusionCpp relies on NVIDIA CUDA for GPU acceleration.

### Operating System
*   **Windows 10/11 x64** (Primary support)
*   *Linux support is currently for development/building from source only.*

### Hardware
*   **NVIDIA GPU**: Required for high-performance inference.
*   **VRAM**: 4GB minimum, 8GB+ recommended for 1080p+ video processing.

### Software Dependencies
You must have the following installed and configured in your system PATH or placed alongside the executable:

| Component | Version Requirement | Tested Version | Note |
| :--- | :--- | :--- | :--- |
| **CUDA Toolkit** | >= 12.2 | 12.8.1 | Required for GPU execution. |
| **cuDNN** | >= 9.2 | 9.19.0 | Deep learning primitives. |
| **TensorRT** | >= 10.2 | 10.15.1 | Optional but recommended for max speed. |

> **Note**: If you download the pre-packaged Release, some dependencies (like FFmpeg) might be included or instructions provided to place DLLs. Ensure CUDA/cuDNN are installed on your system.

---

## 2. Installation

1.  **Download Release**:
    Go to the [Releases](https://github.com/CodingRookie98/faceFusionCpp/releases) page and download the latest `.zip` package for Windows.

2.  **Extract**:
    Unzip the package to a directory (e.g., `D:\FaceFusionCpp`).
    *Avoid paths with non-ASCII characters or spaces if possible.*

3.  **Directory Structure**:
    After extraction, you should see:
    ```
    FaceFusionCpp/
    ├── bin/                # Executables and DLLs
    ├── models/             # AI Models (.onnx)
    ├── resources/          # Font files, masks, etc.
    ├── FaceFusionCpp.exe   # Main application
    └── ...
    ```

---

## 3. First Run (Face Swap)

Let's perform a basic face swap to verify everything is working.

### Preparation
1.  Prepare a **Source Image** (the face you want to use). Let's call it `source.jpg`.
2.  Prepare a **Target Image** (the image you want to change). Let's call it `target.jpg`.
3.  Place them in the project root for simplicity.

### Execution
Open a terminal (PowerShell or CMD) in the installation directory and run:

```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg
```

**Parameters Explained**:
*   `-s, --source`: Path to the face source image.
*   `-t, --target`: Path to the target image or video.
*   `-o, --output`: Path where the result will be saved.

### Expected Output
You should see logs indicating:
1.  **Device initialization** (CUDA/TensorRT).
2.  **Model loading** (inswapper_128, etc.).
3.  **Processing progress**.
4.  **Completion message**.

Check `output.jpg` to see the result!

---

## 4. Troubleshooting

If the application fails to start or crashes:

*   **"DLL not found"**: Ensure CUDA, cuDNN, and TensorRT `bin` directories are in your system PATH, or copy the required `.dll` files next to `FaceFusionCpp.exe`.
*   **"CUDA error"**: Update your NVIDIA GPU drivers to the latest version.
*   **"Model not found"**: Ensure the `models/` directory exists and contains the required `.onnx` files.

For more detailed configuration options, see the [User Guide](user_guide.md) or [CLI Reference](cli_reference.md).
