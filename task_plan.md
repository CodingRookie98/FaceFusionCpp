# Task Plan: P2 Architecture Decoupling Stabilization

## Goal
Fix module import errors for `domain.pipeline.context`, restore `ProcessorFactory` logic in `PipelineRunner`, and achieve a clean build.

## Phases

### Phase 1: Setup & Reproduction
- [x] Initialize planning files (Done)
- [x] Run `python build.py --action build` to reproduce errors
- [x] Analyze `CMakeLists.txt` and module files (`pipeline_context.ixx`, `pipeline_runner.ixx`)

### Phase 2: Fix Module Imports
- [x] Fix `src/domain/pipeline/pipeline_context.ixx` export declaration
- [x] Verify `CMakeLists.txt` includes `pipeline_context.ixx` in the correct target and file set
- [x] Check `src/services/pipeline/pipeline_runner.ixx` imports

### Phase 3: Restore Factory Logic
- [x] Uncomment factory code in `src/services/pipeline/pipeline_runner.cpp`
- [x] Adapt code to new `ProcessorFactory` interface if needed
- [x] Restore initialization of all services (Swapper, Enhancer, Expression, Frame)
- [x] Fix linker issues with `RegisterBuiltinAdapters`

### Phase 4: Verification
- [x] Clean build
- [x] Run unit tests (Tests run, functional failures noted but architecture is stable)

### Phase 5: Functional Debugging
- [x] Fix `PipelineRunnerImageTest` model loading issue (Passed correct path)
- [ ] Fix SEGFAULT in `PipelineRunnerImageTest` (Teardown crash)
- [ ] Debug functional tests (High distance score)
