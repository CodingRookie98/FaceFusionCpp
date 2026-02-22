# Getting Started

Welcome to **FaceFusionCpp**, the high-performance C++ implementation of the popular open-source project [facefusion](https://github.com/facefusion/facefusion). This document will guide you through environment preparation, installation, and your first face-swapping operation.

## 1. System Requirements

Before downloading, please ensure your system meets the following requirements. FaceFusionCpp relies on NVIDIA CUDA for GPU acceleration.

### Operating System
*   **Windows 10/11 x64** (Currently the primary supported platform)
*   *Linux support is currently aimed primarily at development and source code building.*

### Hardware
*   **NVIDIA GPU**: Required for high-performance inference.
*   **VRAM**: Minimum 4GB. Strongly recommended 8GB+ for handling 1080p+ videos.

### Software Dependencies
You must install the following components and add them to your system PATH, or place them in the executable's directory:

| Component | Required Version | Tested Version | Description |
| :--- | :--- | :--- | :--- |
| **CUDA Toolkit** | >= 12.2 | 12.8.1 | Required for GPU execution. |
| **cuDNN** | >= 9.2 | 9.19.0 | Required deep learning primitives. |
| **TensorRT** | >= 10.2 | 10.15.1 | Optional but highly recommended for peak velocity. |

> **Note**: If you downloaded a pre-packaged Release, some dependencies (like FFmpeg) might already be bundled, or shipped with instructions on where to drop your DLLs. Ensure CUDA/cuDNN are properly installed on your system.

---

## 2. Installation Steps

1.  **Download the Release Package**:
    Proceed to our [Releases](https://github.com/CodingRookie98/faceFusionCpp/releases) page to download the latest `.zip` package for Windows.

2.  **Unpack**:
    Extract the ZIP archive into a directory (e.g., `D:\FaceFusionCpp`).
    *Try to avoid paths incorporating non-ASCII characters or excessive spacing.*

3.  **Directory Structure**:
    Once extracted, you should see the following structure:
    ```
    FaceFusionCpp/
    ├── bin/                # Executables and DLLs
    ├── models/             # AI model files (.onnx)
    ├── resources/          # Font files, overlays, and masks
    ├── FaceFusionCpp.exe   # Main application
    └── ...
    ```

---

## 3. First Run (Face Swapping)

Let's execute a basic face-swapping operation to verify that everything is working.

### Preparation
1.  Prepare a **Source Image** (the face you wish to use). Let's assume it is named `source.jpg`.
2.  Prepare a **Target Image** (the picture you wish to overwrite). Let's assume it is named `target.jpg`.
3.  For convenience, place them directly inside the project root directory.

### Execution
Open a terminal (PowerShell or CMD) at the installation directory, and run:

```powershell
.\FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg
```

**Parameter Explanation**:
*   `-s, --source`: The source face image path.
*   `-t, --target`: The target image or video path you're injecting into.
*   `-o, --output`: The path to save the result.

### Expected Output
You should witness the following scroll in the log:
1.  **Device Initialization** (CUDA/TensorRT).
2.  **Model Loading** (inswapper_128, etc.).
3.  **Processing Progress**.
4.  **Completion Message**.

Check `output.jpg` to view your result!

---

## 4. System Check

When encountering environmental problems or before kicking off your first massive run, it is highly recommended to fire off the built-in system checker. This utility rapidly diagnoses the dependencies of CUDA, TensorRT, your Models, and FFmpeg:

```powershell
.\FaceFusionCpp.exe --system-check
```

---

## 5. Troubleshooting

If the application fails to start or crashes abruptly:

*   **"DLL not found"**: Ensure the `bin` directories for CUDA, cuDNN, and TensorRT have been added to your system PATH, or manually copy the required `.dll` files next to `FaceFusionCpp.exe`.
*   **"CUDA error"**: Update your NVIDIA graphics card driver to the newest version.
*   **"Model not found"**: Ensure the `models/` directory exists and houses the required `.onnx` files.

For a significantly more detailed explanation of configuration options, please refer to the [User Guide](user_guide.md) or the [CLI Reference](cli_reference.md).
