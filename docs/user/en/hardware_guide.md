# Hardware Guide

FaceFusionCpp is a high-performance application. This guide helps you choose the optimal configuration based on your hardware (specifically GPU) specs.

---

## 1. GPU Tiers & Strategies

| VRAM Tier | Example GPUs | Memory Strategy | Execution Mode |
| :--- | :--- | :--- | :--- |
| **Flagship (12GB+)** | 4090, 3090 | `tolerant` (Cached) | `sequential` (Fastest) |
| **Mainstream (8-12GB)** | 4070, 3080 | `strict` | `sequential` |
| **Entry (4-8GB)** | 4060, 3060 | `strict` | `batch` (Step-by-step) |
| **Budget (<4GB)** | 1650, 1050 | `strict` | `batch` + `disk` buffer |

---

## 2. Advanced VRAM Optimization

If you encounter **Out of Memory (OOM)** errors, try these steps in order:

1.  **Step-by-Step Batching (`batch` mode)**:
    Set `resource.execution_order: "batch"` in `task_config.yaml`. This allows the app to unload models between stages, ensuring only one model is active on the GPU at any time.

2.  **Lower Concurrency & Buffers**:
    Set `max_queue_size: 5`. This restricts the number of frames held in memory waiting for processing.

3.  **Tile Processing for Frame Enhancement**:
    When running `frame_enhancer` (e.g., Real-ESRGAN), the app automatically enables **Tile Processing** if VRAM is tight. No manual intervention is needed, but reducing `tile_size` (future configurable) significantly cuts peak usage.

4.  **Segmented Video Processing**:
    For cards under 4GB, use `segment_duration_seconds: 5-10` to prevent temporary memory spikes during FFmpeg merging of large files.

---

## 3. Benchmarks (Tested on RTX 4060 8GB)

| Scenario | Task Flow | Resolution | Performance |
| :--- | :--- | :--- | :--- |
| **Static Image** | Swap | 1080p | < 1.5s |
| **Video (Stream)** | Swap only | 1080p | ~10 FPS |
| **Video (Full)** | Swap + Enhancement | 720p | ~6 FPS |

> [!IMPORTANT]
> Enabling **FP16** models (e.g., `inswapper_128_fp16`) significantly reduces VRAM footprint and boosts speed on RTX 20/30/40 series cards.
