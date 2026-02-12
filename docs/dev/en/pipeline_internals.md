# Pipeline Internals

## 1. Architecture

The processing pipeline is the core of FaceFusionCpp. It is designed as a **Linear Chain** of **Processors** connected by **Queues**.

```mermaid
graph LR
    Source[Video Reader] -->|Frame| Q1[Input Queue]
    Q1 --> P1[Face Swapper]
    P1 -->|Frame| Q2[Intermediate Queue]
    Q2 --> P2[Face Enhancer]
    P2 -->|Frame| Q3[Output Queue]
    Q3 --> Sink[Video Writer]
```

### 1.1 Key Components

*   **PipelineRunner**: The orchestrator. It creates threads, initializes processors, and manages the lifecycle.
*   **Processor**: A stateless unit of work (e.g., `FaceSwapper`). It takes a frame, performs AI inference, and returns a modified frame.
*   **Adapter**: A wrapper around a Processor that handles domain logic (e.g., cropping faces, pasting them back). The PipelineRunner actually orchestrates Adapters.
*   **Queue**: A bounded, thread-safe blocking queue. It provides **Backpressure**.

## 2. Execution Flow

### 2.1 Sequential Mode (Default)
In this mode, a single frame flows through the entire pipeline before the next frame enters (conceptually, though pipelining allows some parallelism).

1.  **Reader** decodes Frame 1.
2.  **Swapper** processes Frame 1.
3.  **Enhancer** processes Frame 1.
4.  **Writer** encodes Frame 1.

**Pros**: Low latency, low memory usage.
**Cons**: Frequent model context switching if using the same GPU for different models.

### 2.2 Backpressure Mechanism
If the **Writer** is slow (e.g., encoding 4K video), the Output Queue fills up.
1.  **Enhancer** tries to push to Output Queue -> BLOCKS.
2.  **Swapper** tries to push to Intermediate Queue -> BLOCKS.
3.  **Reader** tries to push to Input Queue -> BLOCKS.

This prevents the system from running out of memory (OOM) by reading the entire video into RAM.

## 3. Shutdown Sequence

Graceful shutdown is critical to avoid corrupted video files.

1.  **Signal**: User presses Ctrl+C or Reader reaches EOF.
2.  **Propagation**:
    *   Reader sends `std::nullopt` (End-of-Stream token) to Q1.
    *   Swapper sees EOS, finishes work, sends EOS to Q2.
    *   Enhancer sees EOS, finishes work, sends EOS to Q3.
    *   Writer sees EOS, finalizes the video file, and closes.
3.  **Timeout**: If the pipeline is stuck (e.g., deadlocked), a watchdog timer forces `std::exit`.
