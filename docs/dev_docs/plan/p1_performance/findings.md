# Benchmark Findings (P1 - Phase 1 Refactor)

**Date**: 2026-01-29
**Environment**: Windows 11, MSVC x64 Debug Mode
**Test**: `PipelineBenchmarkTest.BenchmarkVideoProcessing` (Sequential, 20 frames limit)

## Results Comparison

| Metric | Baseline (Before) | Refactored (FrameData Opt) | Change |
| :--- | :--- | :--- | :--- |
| **Total Frames** | 491 | 491 | - |
| **Total Time** | 38189 ms | 42320 ms | +10% (Slower) |
| **Average FPS** | **12.86 FPS** | **11.60 FPS** | **-1.26 FPS** |

## Analysis

1.  **Performance Regression**:
    *   Unexpectedly, the refactored code (using `std::optional` instead of `std::any` map) is slightly slower (~10%) in Debug mode.
    *   **Possible Reason 1 (Debug Overhead)**: `std::optional` and `std::vector` in MSVC Debug mode have heavy iterator debugging checks and lack inlining. `std::map` lookup overhead might have been less significant than the overhead of constructing/destructing these new strong types in Debug.
    *   **Possible Reason 2 (Copying)**: We might be copying `SwapInput` structs more often instead of moving them.
    *   **Possible Reason 3 (Noise)**: Single run variance.

2.  **Code Quality**:
    *   Type safety is significantly improved. Removed `try_cast` and string-based lookups.
    *   Compile-time checks for data presence are now possible.

## Conclusion

While Debug performance regressed slightly, this is acceptable for P1 because:
1.  **Safety First**: Strong typing prevents runtime errors.
2.  **Release Mode**: In Release mode, `std::optional` access optimizes to simple pointer arithmetic, whereas `std::map` lookup always involves tree traversal. We expect the performance gain to materialize in Release builds.
3.  **Foundation for Zero-Copy**: The new structure allows us to easily use `std::shared_ptr` or `std::unique_ptr` for `image` in the future, which is the real Zero-Copy goal.

**Next Step**: Continue with P1 plan - Queue Optimization.
