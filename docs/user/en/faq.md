# Frequently Asked Questions (FAQ)

This document addresses common installation issues, runtime errors, and performance-related questions.

---

## 1. Installation & Startup

### Q: Startup complains about missing "VCRUNTIME140.dll" or "MSVCP140.dll"

**A**: Please install the [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) (2015-2022 versions).

### Q: Startup complains about missing "cudart64_12.dll" or "nvinfer.dll"

**A**: Ensure you have installed the CUDA Toolkit (12.x) and TensorRT (10.x), and added their `bin` directories to your system's PATH environment variable.
Alternatively, copy the required DLL files directly next to `FaceFusionCpp.exe`.

### Q: The antivirus software reports a virus

**A**: This is a common false positive because the program is not digitally signed. You can add the program directory to your antivirus software's whitelist.

---

## 2. Runtime Errors

FaceFusionCpp uses specific error codes to identify issues. They fall into four categories: System (E100-E199), Configuration (E200-E299), Model (E300-E399), and Runtime (E400-E499).

### System Infrastructure Errors (E100-E199)

* **E101: Out of Memory (OOM)**
  * **Reason**: GPU VRAM exhaustion, typically caused by the producer (decoder) running much faster than the consumer (AI inference).
  * **Solution**:
    1. Enable the `strict` memory strategy in `app_config.yaml`.
    2. Enable **Adaptive Backpressure**: Set a reasonable `max_memory_usage` (e.g., "4GB") in `app_config.yaml`. The system will automatically throttle based on this limit.
    3. Decrease `max_queue_size` (suggest dropping from 20 down to 5 or 10).
* **E102: CUDA Device Not Found/Lost**
  * **Reason**: Driver not installed, version too low, or hardware connection failure.
  * **Solution**: Run `./FaceFusionCpp --system-check` for an environment integrity self-test.
* **E103: Worker Thread Deadlock**
  * **Reason**: Queue resource contention or abnormal system scheduling.
  * **Solution**: Restart the application. If this persists, try reducing `thread_count`.

### Configuration & Initialization Errors (E200-E299)

* **E201 (Invalid YAML Format)**: Verify the syntax in `app_config.yaml` or `task_config.yaml`.
* **E202 (Parameter Out of Bounds)**: Correct erroneous values, e.g., `blend_factor` must remain between 0.0 and 1.0.
* **E203 (Configuration File Not Found)**: Ensure the specified configuration file path is correct.

### Model Resource Errors (E300-E399)

* **E301: Model Load Failed**
  * **Reason**: Corrupted model file or an incompatible version.
* **E302: Model File Missing**
  * **Solution**: Ensure the `models/` directory exists and contains the required `.onnx` files. If `download_strategy` is set to `auto`, they will download automatically.

### Runtime Business Logic Errors (E400-E499)

* **E401 (Image Decode Failed) / E402 (Video Open Failed)**: Check if the input file is corrupted or in an unsupported format.
* **E403 (No Face Detected)**: The program could not detect a face in the current frame. The program will skip processing this frame (passing through the original frame). It will not cause the task to fail.
* **E404 (Face Not Aligned)**: The face angle is too extreme, causing keypoint detection to fail. It will be ignored or retried.

---

## 3. Performance Issues

### Q: Why is the first run so slow?

**A**: During the first run, the program must compile an optimized **TensorRT engine** specifically for your GPU. This process can take **5-10 minutes**.

* Subsequent runs will skip this and directly use the cached engine, starting instantly.
* Ensure `engine_cache.enable: true` is enabled in `app_config.yaml`.

### Q: Why is video processing slow?

**A**: Video processing involves decoding, face detection (per frame), swapping, enhancement, and encoding. This is an immense computational workload.

* **Bottleneck**: Usually the face enhancer (GFPGAN). Disabling it can speed things up by 2-3 times.
* **Resolution**: Processing 4K videos is significantly slower than 1080p.

### Q: How can I speed up processing?

1. **Disable the face enhancer** (if extreme details are unnecessary).
2. **Set a proper memory strategy**: If your VRAM > 8GB, ensure you enable `memory_strategy: tolerant` in `app_config.yaml`.
3. **Tune Thread Count**: Default `thread_count: 0` uses half of your logical cores. On high-core-count CPUs, manually setting this (e.g., 4 or 8) might balance throughput better.
4. **Prefer TensorRT**: Ensure `tensorrt` has the highest priority in `inference.default_providers`.

---

## 4. Quality Issues

### Q: The face in the video keeps flickering

**A**: This is a common issue with frame-by-frame face swapping.

* **Solution**: We are developing a temporal consistency module (planned).
* **Workaround**: Use a higher quality source face, and ensure the target video's face is evenly lit.

### Q: The colors look wrong

**A**: Under extreme lighting conditions, the color matching algorithm may behave erratically.

* We currently use `reinhard` or `hist_match` for color transfer. Future versions will allow users to manually switch algorithms.
