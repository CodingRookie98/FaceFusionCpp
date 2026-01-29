# Progress Log

## 2026-01-29

*   [Completed] Initialize session and planning files.
*   [Completed] Reproduce build errors.
*   [Completed] Fix module imports:
    *   Renamed `services.pipeline.context` to `domain.pipeline.context`.
    *   Fixed `domain_pipeline` CMake target.
    *   Fixed `pipeline_adapters` compilation errors.
*   [Completed] Restore `PipelineRunner` logic:
    *   Re-implemented `AddProcessorsToPipeline` with `ProcessorFactory`.
    *   Added initialization for `FaceAnalysisProcessor`, `FaceSwapper`, `FaceEnhancer`, `ExpressionRestorer`, and `FrameEnhancer`.
    *   Implemented `RegisterBuiltinAdapters` to solve linker symbol stripping issues.
*   [Completed] Verify build: Build successful.
*   [Completed] Test Verification & Debugging:
    *   Initial SegFault in `PipelineRunnerImageTest.ProcessSingleImage` identified as a model loading issue (InSwapper trying to load "default_model").
    *   Fixed `PipelineRunner` to pass correct model paths via context to `PipelineContext`.
    *   Updated `PipelineAdapters` to use these paths instead of "default_model".
    *   Fixed persistent SegFault on test exit by adding explicit destructors to `SwapperAdapter` (likely fixing static destruction order or module boundary issues).
    *   Verified `PipelineRunnerImageTest.ProcessSingleImage` passes successfully.
