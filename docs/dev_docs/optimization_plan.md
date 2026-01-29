# Architectural Optimization Plan

## 1. Modern C++ & Resource Management (Hygiene)
**Priority: P0 (Immediate)**

*   **Objective**: Eliminate legacy patterns to improve stability and safety.
*   **Action Items**:
    1.  **Smart Pointers**: Replace `new/delete` in `ModelRepository`, `InferenceSession`, `PipelineRunner` with smart pointers.
        *   *Guidance*: Default to `std::unique_ptr` for exclusive ownership. Use `std::shared_ptr` ONLY if multiple components share ownership (e.g., cached models).
    2.  **Exception Safety**: Audit `src` for empty `catch` blocks. Add logging (`spdlog::error`) or rethrow (`throw;`).
    3.  **Type Safety**: Replace C-style casts `(int)` with `static_cast<int>`.
    4.  **String Efficiency**: Refactor config parsers and adapters to use `std::string_view` for read-only parameters.

## 2. Concurrency & Performance
**Priority: P1 (High Throughput)**

*   **Objective**: Reduce locking overhead and memory copying in the Pipeline.
*   **Action Items**:
    1.  **False Sharing Fix**: Apply `alignas` padding to `ThreadSafeQueue` head/tail indices.
        *   *Implementation*: Use `alignas(std::hardware_destructive_interference_size)` (C++17) or fallback to `alignas(64)` for cache line isolation.
    2.  **Zero-Copy Data Flow**: Enforce `std::unique_ptr<FramePacket>` semantics to ensure frames are `std::move`d, never copied.
    3.  **Reorder Buffer**: Replace `std::map` (O(log n)) with a pre-allocated **Circular Buffer** (O(1)) for frame reordering.
        *   *Strategy*: Implement a custom lock-free ring buffer or use `boost::circular_buffer` if dependency allows.

## 3. Architecture Decoupling
**Priority: P2 (Maintainability)**

*   **Objective**: Break the "God Object" anti-pattern in `PipelineRunner`.
*   **Action Items**:
    1.  **Processor Factory**: Implement a self-registering factory pattern.
        *   *Mechanism*: Macro-based registration `REGISTER_PROCESSOR(FaceSwapper)` placed in the Processor's `.cpp` file (not header).
        *   *Benefit*: `PipelineRunner` no longer needs to `#include` every processor header.
    2.  **Typed Metadata**: Replace magic strings (`"swap_input"`) in `FrameData` with a strong-typed `FrameMetadata` struct or typesafe variant.
