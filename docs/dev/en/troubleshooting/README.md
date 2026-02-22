# Troubleshooting Knowledge Base

This knowledge base collects common issues encountered during the development process, along with root cause analysis and solutions.

## 1. Issue Categories

For easier retrieval, issues are organized into the following categories:

- **Build**: Compiler versions, missing libraries, vcpkg download failures, etc.
- **Memory**: Out of Memory (OOM), backpressure failure, resource leaks.
- **Device (CUDA/TensorRT)**: Driver incompatibility, operator non-support, engine loading crashes.
- **Logic**: Face detection failure, video sync issues, pipeline hang.

## 2. Issue Index

| Category | Summary | Status | Details |
| :--- | :--- | :--- | :--- |
| Build | C++20 Modules not supported | Solved | [Issue #1](./zh/issues/cpp20_modules.md) |
| Device | TensorRT Myelin crash on exit | Solved | [Issue #2](./zh/issues/trt_myelin_crash.md) |
| Memory | Video test timeout (Strict Memory) | Investigated | [Issue #3](./zh/issues/video_timeout.md) |
| Logic | VideoWriter invalid dimensions | Fixed | [Issue #4](./zh/issues/videowriter_dimensions.md) |

## 3. Contribution Guidelines

If you find a new issue and solve it, please record it in the `zh/issues/` directory (or `en/issues/`) following the [Maintenance Guide](../quickstart.md) and add an index here.
