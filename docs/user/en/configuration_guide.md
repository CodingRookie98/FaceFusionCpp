# Configuration Guide

FaceFusionCpp uses a flexible YAML-based configuration system. There are two main configuration files:

1.  **`app_config.yaml`**: Global application settings (hardware, paths, logging).
2.  **`task_config.yaml`**: Task-specific settings (inputs, processors, parameters).

---

## 1. Application Configuration (`app_config.yaml`)

This file is usually located in the `config/` directory. It controls how the application interacts with your system.

### Structure

```yaml
config_version: "1.0"

inference:
  device_id: 0                  # GPU Device ID (default: 0)
  engine_cache:
    enable: true                # Enable TensorRT engine caching
    path: "./.cache/tensorrt"   # Cache location
    max_entries: 5              # Max cached engines
    idle_timeout_seconds: 60    # Cleanup timeout

resource:
  memory_strategy: "strict"     # "strict" (limit VRAM) or "tolerant"

logging:
  level: "info"                 # trace, debug, info, warn, error
  directory: "./logs"
  rotation: "daily"             # daily, hourly, size
  max_files: 7

models:
  path: "./assets/models"       # Path to AI models
  download_strategy: "auto"     # auto, manual

temp_directory: "./temp"
```

---

## 2. Task Configuration (`task_config.yaml`)

This file defines **what** you want to do (e.g., swap faces in a specific video). You can pass this file using the `-c/--task-config` CLI argument.

### 2.1 Basic Structure

```yaml
config_version: "1.0"

task_info:
  id: "my_first_swap"
  description: "Swapping face from A to B"

io:
  source_paths:
    - "inputs/face_a.jpg"
  target_paths:
    - "inputs/video_b.mp4"
  output:
    path: "outputs/result.mp4"
    video_quality: 80           # 0-100 (default: 80)
    video_encoder: "libx264"    # FFmpeg encoder

resource:
  thread_count: 0               # 0 = auto
  execution_order: "sequential" # sequential, parallel

face_analysis:
  face_detector:
    score_threshold: 0.5
  face_recognizer:
    similarity_threshold: 0.6

pipeline:
  - step: "face_swapper"
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "many" # reference, many, one
      reference_face_path: ""    # Required if mode is 'reference'

  - step: "face_enhancer"
    enabled: true
    params:
      model: "gfpgan_1.4"
      blend_factor: 1.0
```

### 2.2 Pipeline Processors

You can chain multiple processors in the `pipeline` list.

#### **Face Swapper** (`face_swapper`)
Replaces the face in the target with the source face.
*   `model`: Model name (e.g., `inswapper_128_fp16`).
*   `face_selector_mode`:
    *   `many`: Swap all detected faces.
    *   `one`: Swap the largest face.
    *   `reference`: Swap only faces matching `reference_face_path`.

#### **Face Enhancer** (`face_enhancer`)
Restores details in the face area.
*   `model`: Model name (e.g., `gfpgan_1.4`).
*   `blend_factor`: Mix ratio (0.0 - 1.0). 1.0 is full enhancement.

#### **Frame Enhancer** (`frame_enhancer`)
Upscales the entire image/frame.
*   `model`: Model name (e.g., `real_esrgan_x2_fp16`).
*   `enhance_factor`: Intensity (0.0 - 1.0).

---

## 3. Scenarios

### Scenario A: High Quality Video Processing
Enable swapping and enhancement, set high bitrate.

```yaml
io:
  output:
    video_quality: 95
pipeline:
  - step: "face_swapper"
  - step: "face_enhancer"
    params:
      blend_factor: 1.0
  - step: "frame_enhancer" # Optional: super-resolution
```

### Scenario B: Low VRAM Mode
Reduce VRAM usage by processing sequentially and strictly.

```yaml
# In app_config.yaml
resource:
  memory_strategy: "strict"

# In task_config.yaml
resource:
  execution_order: "sequential"
  max_queue_size: 2             # Keep pipeline queue small
```

### Scenario C: Specific Face Replacement
Only replace a specific person's face.

```yaml
pipeline:
  - step: "face_swapper"
    params:
      face_selector_mode: "reference"
      reference_face_path: "inputs/person_to_replace.jpg"
```
