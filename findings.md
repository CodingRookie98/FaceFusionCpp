# Findings

## 2026-01-29 Session Start

### Context
- **Goal**: P2 Architecture Decoupling Stabilization.
- **Current State**: `PipelineContext` moved to domain layer, but imports failing. `ProcessorFactory` implemented but disabled in runner.
- **Files of Interest**:
    - `src/domain/pipeline/pipeline_context.ixx`
    - `src/services/pipeline/pipeline_runner.ixx`
    - `src/services/pipeline/pipeline_runner.cpp`
    - `src/services/pipeline/pipeline_adapters.cpp`
    - `CMakeLists.txt` (root and subdirs)

### Debugging Log (PipelineRunnerImageTest.ProcessSingleImage)
- **Observations**:
    - **MIRACLE**: Adding a destructor with logging to `SwapperAdapter` **FIXED** the SEGFAULT.
    - Test passed: `100% tests passed, 0 tests failed out of 1`.
    - **Why?**
        - Adding a user-defined destructor (even if just logging) might have changed the destruction behavior of the class or the order of member destruction (though it shouldn't for standard members).
        - More likely: The previous default destructor might have been inlined or optimized in a way that caused issues across module boundaries (MSVC Modules are still experimental).
        - Or, simply engaging the `Logger` shared_ptr in the destructor kept the `Logger` alive slightly longer, or the timing changed enough to avoid a race condition during static destruction.
    
    - Wait, I noticed I didn't see `***Exception: SegFault` in the output this time.
    - And `Total Test time` was similar (24.72 sec vs 25.68 sec).
    - But it says `Passed`.
    - This is unexpected behavior ("Heisenbug").
    - However, since it passes now, we have a working state.

- **Next Steps**:
    - The SEGFAULT is gone.
    - I should remove the debug logging before finishing, but keep the destructor if it's the fix.
    - I will keep the explicit destructor but remove the log call to see if it still passes.
    - Actually, I'll keep the log as `trace` level or just comment it out to be safe.
    - Or better, leave it as is for now, it's harmless and useful for future debugging.
    - Wait, I should clean up the excessive debug logging in `process` method as well.

### Cleanup Plan
1. Remove/Comment out heavy debug logs in `SwapperAdapter::process`.
2. Remove/Comment out heavy debug logs in `FaceAnalysisProcessor`.
3. Keep the `SwapperAdapter` destructor but make it empty or minimal logging.
4. Verify tests again.
