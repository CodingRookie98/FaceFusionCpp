# Hardware Guide

FaceFusionCpp is a high-performance application, but its speed and capability depend heavily on your hardware, specifically the GPU. This guide helps you choose the right configuration for your hardware.

---

## 1. GPU Tiers

We categorize hardware into four tiers based on Video RAM (VRAM) and compute capability.

### Tier 1: Flagship (>= 12GB VRAM)
*   **Hardware**: RTX 4090, RTX 3090, RTX 4080 (16GB).
*   **Capabilities**: 
    *   Real-time 4K video processing.
    *   Complex pipelines (Swap + Enhance + SuperRes).
    *   Parallel execution of multiple tasks.
*   **Recommended Settings**:
    *   `memory_strategy`: `tolerant` (Keep models in VRAM).
    *   `execution_order`: `sequential` (Low latency).
    *   `max_queue_size`: 30+.

### Tier 2: Mainstream (8 - 12GB VRAM)
*   **Hardware**: RTX 4070, RTX 3080, RTX 2080 Ti.
*   **Capabilities**:
    *   Standard 1080p/2K video processing.
    *   Swap + Enhance pipelines.
*   **Recommended Settings**:
    *   `memory_strategy`: `strict` (Load/Unload models as needed).
    *   `execution_order`: `sequential`.
    *   `max_queue_size`: 20.

### Tier 3: Entry (4 - 8GB VRAM)
*   **Hardware**: RTX 3060, RTX 4060, RTX 2060.
*   **Capabilities**:
    *   1080p video processing (may require batching).
    *   720p real-time processing.
*   **Recommended Settings**:
    *   `memory_strategy`: `strict`.
    *   `execution_order`: `batch` (Process all frames with one model first, then next).
    *   `batch_buffer_mode`: `disk` (Use disk for intermediate frames to save RAM).

### Tier 4: Low-End (< 4GB VRAM)
*   **Hardware**: GTX 1650, GTX 1050 Ti, MX series.
*   **Capabilities**:
    *   720p or lower resolution.
    *   Not recommended for long videos without segmenting.
*   **Critical Settings**:
    *   `memory_strategy`: `strict`.
    *   `execution_order`: `batch`.
    *   `max_queue_size`: 5-10.
    *   `segment_duration_seconds`: 5 (Process video in 5-second chunks to prevent crashes).

---

## 2. Memory Optimization Strategy

If you encounter **Out of Memory (OOM)** errors, try these steps in order:

1.  **Enable Strict Mode**:
    Set `resource.memory_strategy: "strict"` in `app_config.yaml`. This forces the app to release model resources when not in use.

2.  **Reduce Queue Size**:
    Lower `resource.max_queue_size` in `task_config.yaml` (e.g., from 20 to 5). This reduces the number of frames held in RAM/VRAM.

3.  **Switch to Batch Mode**:
    Set `resource.execution_order: "batch"`. This processes the entire video with Model A, releases Model A, then loads Model B. This ensures only one model is in VRAM at a time.

4.  **Use Disk Buffering**:
    If using Batch mode, set `resource.batch_buffer_mode: "disk"`. This offloads intermediate frames to your SSD instead of RAM.

---

## 3. Performance Expectations

Benchmarks based on **RTX 4060 Laptop (8GB)**:

| Task | Resolution | Time / FPS |
| :--- | :--- | :--- |
| Image Swap | 512x512 | < 1s |
| Image Swap | 1080p | < 2s |
| Video Swap | 720p | > 15 FPS |
| Video Swap | 1080p | > 8 FPS |

> **Note**: Enabling Face Enhancer (GFPGAN) will reduce FPS by approximately 40-60%.
