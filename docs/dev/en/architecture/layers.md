# 5-Layer Architecture Implementation Details

## 1. Architecture Overview

FaceFusionCpp follows a strict unidirectional dependency flow:
`Application -> Services -> Domain -> Platform -> Foundation`

## 2. Layer Details

### 2.1 Layer 1: Application (`src/app/`)
- **AppConfig**: Global environment configuration (GPU ID, Log Level).
- **TaskConfig**: Task-level configuration (Pipeline topology, IO paths).
- **CLI**: A lightweight access layer implemented using CLI11.

### 2.2 Layer 2: Services (`src/services/`)
- **PipelineRunner**: Responsible for orchestrating the producer-consumer model.
- **ShutdownHandler**: Unified management of termination signals like `SIGINT`.
- **CheckpointManager**: Manages resume logic for long video removals.

### 2.3 Layer 3: Domain (`src/domain/`)
- **FaceModelRegistry**: Manages the lifecycle of all AI models.
- **Image/Video Wrappers**: Business logic wrappers for OpenCV and FFmpeg.
- **Processors**: Core business logic for normalized image inference.

### 2.4 Layer 4: Platform (`src/platform/`)
- **FileSystem (fs)**: Provides cross-platform file operations and path normalization.
- **Threading**: Abstracts OS thread priorities.

### 2.5 Layer 5: Foundation (`src/foundation/`)
- **InferenceEngine**: Deep wrapper for TensorRT and ONNX Runtime.
- **Logger**: Enhanced logging system based on `spdlog`.
- **Utilities**: Backpressure semaphores, UUID generators.

## 3. Dependency Rules

1. **No Layer Skipping**: Application must not directly call private Domain interfaces; it must use Services orchestration.
2. **No Circular Dependencies**: If a circular dependency is found, consider extracting common logic to the Foundation layer.
3. **Interface First**: Modules communicate via interfaces defined in `.ixx`, hiding internal implementation (PIMPL).
