# Frequently Asked Questions (FAQ)

This document addresses common issues, errors, and performance questions.

---

## 1. Installation & Startup

### Q: I get "VCRUNTIME140.dll" or "MSVCP140.dll" missing error.
**A**: Install the [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) for Visual Studio 2015-2022.

### Q: I get "cudart64_12.dll" or "nvinfer.dll" not found.
**A**: Ensure you have installed CUDA Toolkit (12.x) and TensorRT (10.x). Add their `bin` directories to your system PATH environment variable.
Alternatively, copy the required DLLs to the same folder as `FaceFusionCpp.exe`.

### Q: My antivirus flags the executable as a threat.
**A**: This is likely a false positive because the application is not digitally signed. You can add an exception for the application folder.

---

## 2. Runtime Errors

FaceFusionCpp uses specific error codes to help identify issues.

### E101: Out of Memory (OOM)
*   **Cause**: Your GPU VRAM is full.
*   **Solution**:
    1.  Switch to `strict` memory strategy in `app_config.yaml`.
    2.  Use `batch` execution order with `disk` buffering (see [Hardware Guide](hardware_guide.md)).
    3.  Lower `max_queue_size`.

### E301 / E302: Model Errors
*   **Cause**: Model files are missing, corrupted, or incompatible.
*   **Solution**:
    1.  Check if `assets/models` contains the required `.onnx` files.
    2.  Delete the corrupted model and restart the application (if `download_strategy` is `auto`).

### E4xx: Runtime Errors
*   **E403 (No Face Detected)**: The application could not find a face in the frame. It will skip processing for this frame (pass-through).
*   **E404 (Face Not Aligned)**: The face angle is too extreme for the landmark detector.

---

## 3. Performance

### Q: Why is the first run so slow?
**A**: On the first run, the application compiles **TensorRT Engines** optimized for your specific GPU. This process can take **5-10 minutes**.
*   Subsequent runs will use the cached engine and start instantly.
*   Ensure `engine_cache.enable: true` is set in `app_config.yaml`.

### Q: Why is video processing slow?
**A**: Video processing involves decoding, face detection (per frame), swapping, enhancing, and encoding.
*   **Bottleneck**: Usually Face Enhancer (GFPGAN). Disabling it will double or triple the speed.
*   **Resolution**: Processing 4K video is significantly slower than 1080p.

### Q: How to make it faster?
1.  **Disable Face Enhancer** if not strictly necessary.
2.  **Use `many` face selector** only if needed (it processes all faces).
3.  **Upgrade GPU**: An RTX 4090 is ~3x faster than an RTX 3060.

---

## 4. Quality

### Q: The face flickers in the video.
**A**: This is common in frame-by-frame swapping.
*   **Solution**: We are working on a temporal consistency module (Planned).
*   **Workaround**: Use a higher quality source face and ensure the target face is well-lit.

### Q: The colors look wrong.
**A**: The color matching algorithm might fail in extreme lighting.
*   We use `reinhard` or `hist_match` color transfer. Future versions will allow selecting the algorithm.
