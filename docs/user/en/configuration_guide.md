# Configuration Guide

FaceFusionCpp uses a flexible YAML-based configuration system. There are two main configuration files:

1.  **`app_config.yaml`**: Global application settings (hardware, paths, logging, observability).
2.  **`task_config.yaml`**: Task-specific settings (I/O strategy, pipeline topology, algorithm parameters).

---

## 1. Application Configuration (`app_config.yaml`)

This file is usually located in the `config/` directory. It defines the runtime environment and infrastructure.

### Structure and Parameters

```yaml
config_version: "0.34.0"

# Inference Infrastructure
inference:
  device_id: 0                  # GPU Device ID (default: 0)
  engine_cache:
    enable: true                # Enable engine caching (TensorRT)
    path: "./.cache/tensorrt"   # Cache location (relative to root)
    max_entries: 3              # Max cached engines (LRU)
    idle_timeout_seconds: 60    # Auto-release idle engines (seconds)
  default_providers:            # Execution provider priority
    - tensorrt
    - cuda
    - cpu

# Resources & Performance
resource:
  # Memory Strategy:
  #   strict: Releases resources when not in use, best for low VRAM.
  #   tolerant: Preloads and keeps models in VRAM, best for high-frequency tasks.
  memory_strategy: "strict"
  # Global memory quota for backpressure flow control (e.g., "4GB", "2048MB").
  max_memory_usage: "4GB"

# Logging System
logging:
  level: "info"                 # trace, debug, info, warn, error
  directory: "./logs"
  rotation: "daily"             # daily, hourly, size
  max_files: 7                  # Keep last N files
  max_total_size: "1GB"         # Total log size limit

# Observability (Metrics Tracking)
metrics:
  enable: true
  step_latency: true            # Record latency for each step
  gpu_memory: true              # Record GPU memory curves
  report_path: "./logs/metrics_{timestamp}.json"

# Model Management
models:
  path: "./assets/models"
  download_strategy: "auto"     # auto (download if missing), skip (fail), force (re-download)

# Temporary File Directory
temp_directory: "./temp"

# Default Task Settings (Fallback Mechanism)
# These values are used if fields are missing in task_config.yaml
default_task_settings:
  io:
    output:
      video_encoder: "libx264"
      video_quality: 80
      conflict_policy: "error"
```

---

## 2. Task Configuration (`task_config.yaml`)

Defines specific business logic. Can be loaded dynamically via the `-c/--task-config` parameter.

### 2.1 Basic Structure

*   **`task_info`**: Task metadata. Supports `enable_logging` (task-specific log) and `enable_resume` (checkpointing).
*   **`io`**: `source_paths` and `target_paths`. Supports images, videos, and directory scanning.
*   **`io.output`**:
    *   `path`: Export directory (forced absolute path).
    *   `conflict_policy`: `overwrite`, `rename`, `error`.
    *   `audio_policy`: `copy` (keep audio), `skip` (mute).
*   **`resource`**:
    *   `execution_order`:
        *   `sequential`: Frame-by-frame processing (low latency).
        *   `batch`: Chunk-based batching (minimizes VRAM peaks, ideal for budget GPUs).
    *   `batch_buffer_mode`: `memory` or `disk` (use SSD for storage to support long videos).
    *   `segment_duration_seconds`: Video segmentation length to prevent crashes.

### 2.2 Face Analysis (`face_analysis`)

Configures global face detection, landmarking, and recognition.

```yaml
face_analysis:
  face_detector:
    models: ["yoloface", "retinaface"] # Fusion detection strategy
    score_threshold: 0.5
  face_landmarker:
    model: "2dfan4"
  face_recognizer:
    model: "arcface_w600k_r50"
    similarity_threshold: 0.6          # Identity mapping filter
  face_masker:
    types: ["box", "occlusion", "region"] # Mask fusion
    region: ["skin", "nose", "mouth"]      # Targeted processing regions
```

### 2.3 Pipeline Processors (`pipeline`)

A chain of `step` units executed in survival order.

#### **Face Swapper** (`face_swapper`)
*   `model`: `inswapper_128_fp16`, etc.
*   `face_selector_mode`: `reference` (target identity), `many` (all faces), `one` (largest face).

#### **Face Enhancer** (`face_enhancer`)
*   `model`: `codeformer`, `gfpgan_1.4`, etc.
*   `blend_factor`: 0.0 - 1.0 (original vs. enhanced mix ratio).

#### **Expression Restorer** (`expression_restorer`)
*   `model`: `live_portrait`.
*   `restore_factor`: 0.0 - 1.0.

#### **Frame Enhancer** (`frame_enhancer`)
*   `model`: `real_esrgan_x4`, etc.
*   **Tile Processing**: The app automatically tiles large frames (e.g., 4K) to prevent OOM errors.

---

## 3. Global Step Parameters (`global_pipeline_step_params`)

If multiple steps use the same parameters (like `reference_face_path`), define them globally:

```yaml
global_pipeline_step_params:
  face_selector_mode: "reference"
  reference_face_path: "D:/assets/my_face.jpg"
```
