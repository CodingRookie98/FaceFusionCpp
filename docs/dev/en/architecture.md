# System Architecture

## 1. Overview (5-Layer Architecture)

FaceFusionCpp follows a **Strict 5-Layered Architecture** designed for high cohesion and low coupling. This structure ensures that business logic is separated from infrastructure concerns, making the system testable and maintainable.

```mermaid
graph TD
    subgraph "Layer 1: Application (Entry Point & Config)"
        App[CLI / AppConfig / TaskConfig]
    end

    subgraph "Layer 2: Services (Orchestration)"
        Svc[Pipeline Runner / Shutdown Handler / Checkpoint Manager]
    end

    subgraph "Layer 3: Domain (Business Logic)"
        Dom[Face Analysis / Image Processing / Video Handling / Model Registry]
    end

    subgraph "Layer 4: Platform (Hardware Abstraction)"
        Plat[File System / OS APIs / Time]
    end

    subgraph "Layer 5: Foundation (Core Infrastructure)"
        Fdn[Logger / Error Handling / AI Inference Engine (ORT/TensorRT) / Utilities]
    end

    App --> Svc
    Svc --> Dom
    Dom --> Plat
    Plat --> Fdn
    
    %% Cross-layer dependencies (Allowed: Downward only)
    App --> Fdn
    Svc --> Fdn
    Dom --> Fdn
```

### Dependency Rule
*   **Unidirectional**: Higher layers depend on lower layers.
*   **No Cycles**: Circular dependencies are strictly forbidden.
*   **No Up-calls**: Lower layers never call higher layers directly (use callbacks or observers if needed).

---

## 2. Layer Details

### 2.1 Application Layer (`src/app/`)
*   **Responsibility**: Application entry point, configuration parsing, and user interaction.
*   **Key Modules**:
    *   `app.cli`: Handles command-line arguments (CLI11).
    *   `config`: Parses YAML configurations (`app_config.yaml`, `task_config.yaml`).

### 2.2 Services Layer (`src/services/`)
*   **Responsibility**: Orchestrates domain objects to execute complex workflows. It manages the lifecycle of long-running tasks.
*   **Key Modules**:
    *   `services.pipeline`: Manages the processing pipeline (Producer-Consumer model).
    *   `services.shutdown`: Handles graceful shutdown signals (SIGINT).

### 2.3 Domain Layer (`src/domain/`)
*   **Responsibility**: Encapsulates core business rules and domain logic. This layer knows "what" a face is, but not "how" to draw it on a specific OS.
*   **Key Modules**:
    *   `domain.face`: Face detection, recognition, and swapping algorithms.
    *   `domain.image`: Image IO and processing utilities.
    *   `domain.video`: Video frame extraction and encoding.
    *   `domain.ai`: Model repository management.

### 2.4 Platform Layer (`src/platform/`)
*   **Responsibility**: Abstracts operating system specifics (File IO, Threading primitives if custom).
*   **Key Modules**:
    *   `platform.fs`: File system operations.

### 2.5 Foundation Layer (`src/foundation/`)
*   **Responsibility**: Provides low-level utilities used across the entire system.
*   **Key Modules**:
    *   `foundation.ai`: Inference engine wrapper (ONNX Runtime / TensorRT).
    *   `foundation.infrastructure`: Logger (spdlog), Error handling, UUID generation.

---

## 3. Key Design Decisions

### 3.1 C++20 Modules
We use C++20 Modules (`.ixx` interface, `.cpp` implementation) to:
*   **Speed up compilation**: By avoiding header parsing overhead.
*   **Physical Isolation**: Changes in `.cpp` implementation do not trigger recompilation of dependent modules.

### 3.2 PIMPL Idiom (Pointer to Implementation)
Used extensively in the `Foundation` and `Domain` layers to hide implementation details (like 3rd-party library headers) from the public API.
*   **Benefit**: ABI stability and faster build times.

### 3.3 Factory Pattern
Processors (e.g., `FaceSwapper`, `FaceEnhancer`) are created via factories based on configuration strings.
*   **Benefit**: Supports the "Open/Closed Principle". New processors can be added without modifying the consumer code.

### 3.4 RAII (Resource Acquisition Is Initialization)
All resources (CUDA memory, file handles, inference sessions) are managed by RAII classes.
*   **Benefit**: Prevents resource leaks and ensures exception safety.
*   **Example**: `InferenceSession` automatically releases TensorRT engines upon destruction.

### 3.5 Thread-Safe Queues & Backpressure
The pipeline uses bounded thread-safe queues between steps.
*   **Backpressure**: If a consumer (e.g., GPU Inference) is slow, the queue fills up, blocking the producer (e.g., Video Decoder). This prevents Out-Of-Memory (OOM) errors.
