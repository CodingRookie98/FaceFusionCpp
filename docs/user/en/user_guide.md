# User Guide

This guide details the core features of FaceFusionCpp and how to use them effectively for image and video processing tasks.

---

## 1. Core Features

FaceFusionCpp is built around a modular pipeline architecture. The core processing units are called **Processors**. You can use them individually or combine them.

### 1.1 Face Swapper (`face_swapper`)
The core function of the application. It detects faces in the Target image/video and replaces them with the face from the Source image.
*   **Key Parameter**: `face_selector_mode`
    *   `many` (Default): Swaps all detected faces.
    *   `one`: Swaps only the largest face.
    *   `reference`: Swaps only faces that look like the one in `reference_face_path`.

### 1.2 Face Enhancer (`face_enhancer`)
Restores details and sharpness to faces using GFPGAN/CodeFormer. This is highly recommended after face swapping, as the swap model output is typically 128x128 resolution.
*   **Key Parameter**: `blend_factor` (0.0 - 1.0). Controls how much of the enhanced face is blended with the original.

### 1.3 Frame Enhancer (`frame_enhancer`)
Upscales the entire image or video frame using Real-ESRGAN. Useful for improving low-resolution targets.
*   **Key Parameter**: `enhance_factor`.

---

## 2. Workflows

### 2.1 Image Processing
The simplest workflow.
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.png
```
*   **Input**: `target.jpg` (or png, bmp).
*   **Output**: `output.png`. Format is determined by the output filename extension.

### 2.2 Video Processing
The application automatically handles video inputs.
```powershell
FaceFusionCpp.exe -s source.jpg -t target.mp4 -o output.mp4
```
*   **Audio**: By default, audio from the target video is copied to the output.
*   **Performance**: Video processing is resource-intensive. See [Hardware Guide](hardware_guide.md) for optimization.

### 2.3 Batch Processing
For processing multiple files or complex pipelines, use a Configuration File.

1.  Create `my_task.yaml` (see [Configuration Guide](configuration_guide.md)).
2.  Run:
    ```powershell
    FaceFusionCpp.exe -c my_task.yaml
    ```
3.  **Directory Input**: You can specify a directory for `source_paths` or `target_paths` in the config. The application will process all valid media files in that directory.

---

## 3. Processor Combination

The real power comes from combining processors. The order matters!

### Recommended Pipeline: Swap -> Enhance
This ensures the swapped face (which might be low-res) is enhanced before the final output.

**Command Line**:
```powershell
FaceFusionCpp.exe ... --processors face_swapper,face_enhancer
```

**YAML Config**:
```yaml
pipeline:
  - step: "face_swapper"
    params:
      model: "inswapper_128_fp16"
  - step: "face_enhancer"
    params:
      blend_factor: 1.0
```

### Advanced Pipeline: Swap -> Enhance -> Upscale
1.  **Swap**: Replace face.
2.  **Enhance Face**: Fix face details.
3.  **Enhance Frame**: Upscale the whole image (e.g., 2x or 4x).

```yaml
pipeline:
  - step: "face_swapper"
  - step: "face_enhancer"
  - step: "frame_enhancer"
    params:
      model: "real_esrgan_x2_fp16"
```
