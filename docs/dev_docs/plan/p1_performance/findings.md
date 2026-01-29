# Benchmark Findings (P1 - Baseline)

**Date**: 2026-01-29
**Environment**: Windows 11, MSVC x64 Debug Mode (Warning: Debug build significantly impacts performance)
**Test**: `PipelineBenchmarkTest.BenchmarkVideoProcessing` (Sequential, 20 frames limit)

## Results

| Metric | Value |
| :--- | :--- |
| **Total Frames** | 491 (Full video processing) |
| **Total Time** | 38189 ms |
| **Average FPS** | **12.86 FPS** |

## Analysis

1.  **Workload**:
    *   Face Detection (YOLO)
    *   Face Recognition (ArcFace) - for embedding extraction
    *   Face Swapping (InSwapper 128 FP16)
    *   Face Enhancement (GFPGAN 1.4)
    *   Video Decoding/Encoding (H.264)

2.  **Observations**:
    *   Performance is decent for Debug mode (12.8 FPS), likely due to GPU acceleration (CUDA/TensorRT) doing the heavy lifting in ONNX Runtime.
    *   CPU bottleneck (Debug mode overhead) is likely in the glue code: `FrameData` copying, `std::any` casting, `cv::Mat` conversions.
    *   `ThreadSafeQueue` locking overhead might be minimal in Sequential mode but will be significant in Batch mode.

## Optimization Targets (P1)

1.  **FrameData Refactor**: Eliminate `std::map<string, any>` lookups. Direct access should improve CPU instruction cache locality.
2.  **Zero Copy**: Ensure `cv::Mat` is not deep-copied during pipeline stages.
3.  **Release Build**: True performance potential is hidden. Need to verify Release build behavior later.

**Next Step**: Execute `C++_task_framedata_refactor.md`.
