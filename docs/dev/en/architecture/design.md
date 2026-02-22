# Application Layer Architecture Design (V2.8)

## 1. Design Philosophy

FaceFusionCpp is a high-performance face swapping application and library designed for efficiency, modularity, and cross-platform compatibility.

### 1.1 Core Principles
- **Performance First**: Native C++ implementation with TensorRT/CUDA acceleration.
- **Asynchronous Pipeline**: Producer-consumer model with backpressure management.
- **Strict Layering**: 5-layer architecture with unidirectional dependencies.
- **Developer Friendly**: Unified Python build script and clear documentation.

---

## 2. Configuration System

The system uses a hierarchical configuration approach using YAML for persistence.

### 2.1 App Configuration (`app_config.yaml`)
Global settings that persist across different tasks:
- `log_level`: (DEBUG, INFO, WARN, ERROR)
- `model_path`: Base directory for AI models.
- `gpu_id`: Target NVIDIA GPU for execution.

### 2.2 Task Configuration (`task_config.yaml`)
Specific to a single execution run:
- **IO**: Input/Output paths.
- **Pipeline**: List of processors (e.g., `face_swapper`, `face_enhancer`).
- **Memory Strategy**: `strict` (low VRAM) or `tolerant` (high performance).

---

## 3. Core Architecture

### 3.1 5-Layer Dependency Rule
`Application -> Services -> Domain -> Platform -> Foundation`
Refer to [Architecture Layers](../architecture/layers.md) for implementation details.

### 3.2 Pipeline Model
Implemented as a `std::queue` based producer-consumer system with:
- **Parallel Decoding**: FFmpeg threads decoding frames into a pool.
- **Inference Batching**: Grouping frames for optimal TensorRT throughput.
- **Backpressure**: Semaphore-based capping of the frame queue to prevent OOM.

---

## 4. Engineering Constraints

### 4.1 Path Resolution
All internal paths are resolved relative to `FACEFUSION_HOME` using the `platform.fs` module to ensure portability between Windows (UNC paths) and Linux.

### 4.2 Resource Management (RAII)
- No raw `new`/`delete`.
- Smart pointers for inference engines.
- `std::unique_ptr` for PIMPL implementations to hide third-party header noise.

---

## 5. Metrics & Diagnostics

The system generates detailed performance reports in JSON format upon task completion:
- **Throughput**: Mean FPS and P99 latency.
- **Hardware**: Peak VRAM usage and CPU utilization.
- **Status**: Per-processor execution breakdown.

---

## 6. Roadmap
- **ABI Stable Plugins**: Dynamic loading of external processors.
- **REST API Subsystem**: Decoupling the CLI from the backend for server usage.
- **Advanced Batching**: Dynamic batch size adjustment based on VRAM availability.
