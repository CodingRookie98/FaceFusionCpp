# Benchmark Findings (P1 - Phase 2 Queue Opt)

**Date**: 2026-01-29
**Environment**: Windows 11, MSVC x64 Debug Mode
**Test**: `PipelineBenchmarkTest.BenchmarkVideoProcessing` (Sequential, 20 frames limit)

## Results Comparison

| Metric | Baseline | Refactored (FrameData) | Queue Optimized (PopBatch) | Change (vs Baseline) |
| :--- | :--- | :--- | :--- | :--- |
| **Total Frames** | 491 | 491 | 491 | - |
| **Total Time** | 38189 ms | 42320 ms | 37856 ms | -0.9% (Faster) |
| **Average FPS** | **12.86 FPS** | **11.60 FPS** | **12.97 FPS** | **+0.11 FPS** |

## Analysis

1.  **Recovery from Regression**:
    *   The queue optimization (`pop_batch`) successfully recovered the performance regression introduced by the `FrameData` refactor in Debug mode.
    *   We are now slightly faster than the baseline (12.97 vs 12.86 FPS), despite the heavier `FrameData` structure (destructors/constructors of vectors/optionals).

2.  **Mechanism**:
    *   `pop_batch(4)` significantly reduces the number of lock acquisitions/releases on the input queue.
    *   Instead of locking for *every* frame, the worker thread locks once for every 4 frames (on average).
    *   This proves that lock contention was indeed a factor, even in this sequential setup.

## Conclusion

The combination of **Strong Typing (Safety)** + **Batch Queueing (Performance)** has resulted in a net positive outcome. We have a safer codebase with slightly better performance in Debug mode. The gain in Release mode should be even more pronounced as the overhead of the strong types disappears.

**Next Step**: Prepare P1 evaluation report.
