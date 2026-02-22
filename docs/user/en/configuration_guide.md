# Configuration Guide

FaceFusionCpp uses a flexible YAML configuration system. There are two main configuration files:

1. **`app_config.yaml`**: Global application settings (hardware foundation, paths, logging, observability).
2. **`task_config.yaml`**: Task-specific settings (I/O strategy, pipeline topology, algorithm parameters).

---

## 1. Application Configuration (`app_config.yaml`)

This file is usually located in the `config/` directory. It defines the runtime environment and infrastructure. **These settings apply globally to the entire application.**

### Structure and Parameters (Beginner's Guide)

```yaml
config_version: "0.34.0"

# --- Inference Infrastructure (Graphics Card Settings) ---
inference:
  device_id: 0                  # GPU Device ID (Default: 0. Leave as 0 if you only have one dedicated GPU).
  engine_cache:
    enable: true                # Enable inference engine cache (Default: true. Greatly speeds up startup from the 2nd run).
    path: "./.cache/tensorrt"   # Cache location (relative to root).
    max_entries: 3              # Max cache entries (Default: 3. Tells VRAM how many models to retain in cache).
    idle_timeout_seconds: 60    # Auto-release time after idle (Default: 60s. How long before freeing VRAM when a model is unused).
  default_providers:            # Default inference backend priority (Default: tensorrt > cuda > cpu).
    - tensorrt
    - cuda
    - cpu

# --- Resource & Performance (How to manage RAM/VRAM) ---
resource:
  # Memory strategy (Default: "strict")
  #   strict: "Throw away after use" mode. Loads models into VRAM only at the exact second of face swapping. Ideal for low VRAM machines (e.g., <8GB).
  #   tolerant: "Resident memory" mode. Loads everything upfront. Ideal for high-end setups (12GB+) that demand extreme processing speed.
  memory_strategy: "strict"

  # Global memory quota (Default: "4GB"). Set a hard cap if you frequently experience Out of Memory crashes (e.g., "4GB", "2048MB").
  max_memory_usage: "4GB"

# --- Logging (Where to look if things go wrong) ---
logging:
  level: "info"                 # Log level. Supports trace, debug, info, warn, error (Default: "info").
  directory: "./logs"           # Where logs are saved.
  rotation: "daily"             # How often to create a new log file (Default: "daily").
  max_files: 7                  # How many old log files to keep (Default: 7).
  max_total_size: "1GB"         # Maximum total size for all logs combined (Default: "1GB").

# --- Metrics (Tools to plot performance charts) ---
metrics:
  enable: true                  # Enable tracking (Default: true).
  step_latency: true            # Log exactly how many milliseconds each step takes (Default: true).
  gpu_memory: true              # Track VRAM usage curve (Default: true).
  report_path: "./logs/metrics_{timestamp}.json"  # Where the report is saved.

# --- Model Management ---
models:
  path: "./assets/models"
  # What if a model is missing? (Default: "auto" - download automatically)
  # Other options: "skip" (skip and throw error, if you prefer manual downloads), "force" (force re-download everything).
  download_strategy: "auto"

temp_directory: "./temp"        # Temporary file directory.

# --- Default Task Settings (Fallback Mechanism) ---
# When you forget to specify these params in a face-swapping task, the program uses these defaults.
default_task_settings:
  io:
    output:
      video_encoder: "libx264"  # Video encoder (Default "libx264" for best compatibility across most players).
      video_quality: 80         # Video quality (Default 80. Range 0-100, higher is clearer but file is larger).
      prefix: "result_"         # Prefix automatically added to the generated filename (Default "result_").
      suffix: ""                # Suffix automatically added to the generated filename (Default empty).
      conflict_policy: "error"  # What to do on filename collision (Default "error". Can use "overwrite" to replace).
      audio_policy: "copy"      # What to do with the video audio (Default "copy" to keep original audio. Can use "skip" to mute).
```

---

## 2. Task Configuration (`task_config.yaml`)

This file defines the **specific task** you want to execute (e.g., swapping a face in a particular video). You can pass this file via the `-c/--task-config` command line argument.

### 2.1 Basic Structure

* **`task_info`**: Task metadata. Supports `enable_logging` (independent logging, default `false`) and `enable_resume` (resume from breakpoint, default `false`. E.g., if a long video crashes, it can pick up where it left off).
* **`io`**: Input sources (`source_paths`) and targets (`target_paths`). Supports images, videos, and directory scanning.
* **`io.output`**:
    > [!TIP]
    > If you omit these parameters, they will automatically fall back to the `default_task_settings` defaults defined in `app_config.yaml`.

  * `path`: Output directory (must be absolute path).
  * `prefix`: Output filename prefix (Default `result_`).
  * `suffix`: Output filename suffix (Default empty).
  * `conflict_policy`: `overwrite`, `rename`, `error` (Default `error`).
  * `audio_policy`: `copy` (keeps original track, Default `copy`), `skip` (mutes output).
* **`resource`**:
  * `thread_count`: Concurrent task thread count (Default `0`, lets program decide, usually half your CPU thread count).
  * `max_queue_size`: Max queue capacity, buffers to prevent VRAM overflow. (Default `20`. If you get constant OOM errors, drop this to 10 or 5).
  * `execution_order`:
    * `sequential` (Default): Processes frame by frame in order. **Recommended for most beginners.**
    * `batch`: Breaks steps into batches, drastically dropping peak VRAM. **Only recommended for extreme low-end VRAM (<=4GB) setups or those with massive SSD drives.**
  * `batch_buffer_mode`: `memory` (saves to RAM, fast) or `disk` (saves to disk to prevent RAM blowouts, slower but stable).
  * `segment_duration_seconds`: Video segment processing length (Default `0` no segmentation. Can set to minutes for ultra-long videos).

### 2.2 Face Analysis (`face_analysis`)

Configures global parameters for how faces are detected and cropped. All algorithms here **do not affect how the final face looks**, only how accurately it is found.

```yaml
config_version: "1.0"

task_info:
  id: "my_first_swap"
  description: "Swap face A into video B"

io:
  source_paths:
    - "inputs/face_a.jpg"       # Source face image
  target_paths:
    - "inputs/video_b.mp4"      # Target video
  output:
    path: "outputs/result.mp4"
    prefix: "result_"
    suffix: "_v1"

resource:
  thread_count: 0
  max_queue_size: 20
  execution_order: "sequential"

face_analysis:
  face_detector:
    models: ["yoloface", "retinaface"] # Try yolo first, falback to retina.
    score_threshold: 0.5        # Detection confidence (Default 0.5. Lowering to e.g. 0.3 finds blurry faces but might mistake leaves as faces. Raising to 0.8 is highly accurate but misses blurry side profiles).
  face_landmarker:
    model: "2dfan4"             # Model to finding 68 facial keypoints. (Default 2dfan4, most stable right now).
  face_recognizer:
    model: "arcface_w600k_r50"  # Model used to compare if two faces belong to the same person.
    similarity_threshold: 0.6   # Similarity threshold (Default 0.6. Takes effect under 'reference' mode. 0.7 is extremely strict, 0.4 swaps almost any face).
  face_masker:
    # Mask fusion strategies. How to seamlessly stitch the face back on without wiping out blocking hair or glasses.
    types: ["box", "occlusion", "region"]
    occluder_model: "xseg"                # Model to mask out hands/objects in front of the face.
    parser_model: "bisenet_resnet_34"     # Model to parse out facial features.
    region: ["skin", "nose", "mouth"]     # Default "all". Beginners should leave this untouched.
```
### 2.3 Pipeline Processors (`pipeline`)

The core business logic! These are the "workers" (processors) that execute the tasks. They consist of a series of `step`s, **passing the frame sequentially down the line based on the order you configure them**.

> [!NOTE]
> If you omit parameters inside `params`, they will automatically inherit values from [Global Pipeline Step Params](#3-global-pipeline-step-parameters-global_pipeline_step_params) or their respective hard defaults.

#### **Face Swapper** (`face_swapper`)

The most important worker: Replaces the face in the target with the source face.

* `model`: Model name, supports `inswapper_128` or `inswapper_128_fp16`. (Default `inswapper_128_fp16`. Any `fp16` suffix runs faster on most cards with 0 visual difference).
* `face_selector_mode`: (Default `many`)
  * `many`: Swaps **all** detected faces in the frame.
  * `one`: Only swaps the **largest** face in the frame (the protagonist), ignoring background actors.
  * `reference`: Only swaps faces that closely match the photo provided via `reference_face_path` (tells the program exactly "who is who").

#### **Face Enhancer** (`face_enhancer`)

Restores facial details and mosaics. (Since face swapping usually outputs low-res 128x128 faces, this is a required step for HD).

* `model`: Model name, supports `codeformer`, `gfpgan_1.2`~`1.4`. (Default `gfpgan_1.4`. Try `codeformer` if the source looks severely broken).
* `blend_factor`: The blend ratio between the enhanced face and original face (0.0 - 1.0). (Default `0.8`. 1.0 creates a flawless but artificial 3D look. 0.8 retains roughly 20% of the original photo's lighting atmosphere, looking most natural).

#### **Expression Restorer** (`expression_restorer`)

Corrects the stiffly cropped swapped face so that the eyeballs and micro-expressions perfectly match the original target's mood.

* `model`: Model name, supports `live_portrait`. (Default `live_portrait`).
* `restore_factor`: Restoration ratio (0.0 - 1.0). (Default `0.8`. Best left at default).

#### **Frame Enhancer / Super Res** (`frame_enhancer`)

Makes an entire blurry old photo or video crisp (including the background). A highly resource-intensive step.

* `model`: Model name, supports various multipliers like `real_esrgan_x4_fp16`.
* `enhance_factor`: Unsharp mask intensity on output frames. (Default `1.0`).
* **Tile Chunking Strategy**: The app automatically splits the frame into tiles based on your VRAM to prevent 4K videos from OOM crashing, usually requiring zero manual tweaks.

---

## 3. Global Pipeline Step Parameters (`global_pipeline_step_params`)

If you want to only swap a specific person, you don't have to copy-paste the same long directory path into every single `face_enhancer` and `face_swapper` param block. You can define the default reference target globally:

```yaml
# This is placed at the same indentation level as `pipeline`
global_pipeline_step_params:
  # Globally specify: only look for referenced objects when selecting faces
  face_selector_mode: "reference"
  # Set the picture of the target object. All steps default to this configuration.
  reference_face_path: "D:/assets/my_face.jpg"
```

---

## 4. Scenario Configuration Examples

### Scenario A: High-Quality Video Processing

Enables face swapping and enhancement, exporting a high quality video encode.

```yaml
io:
  output:
    video_quality: 95
pipeline:
  - step: "face_swapper"
  - step: "face_enhancer"
    params:
      blend_factor: 1.0
  - step: "frame_enhancer" # Optional: Full frame super resolution
```

### Scenario B: Low VRAM Mode

Reduces VRAM usage via sequential execution and strict memory strategies.

```yaml
# In app_config.yaml
resource:
  memory_strategy: "strict"

# In task_config.yaml
resource:
  execution_order: "sequential"
  max_queue_size: 2             # Keep the pipeline queue small
```

### Scenario C: Specific Face Replacement

Only swaps a specific face within the video.

```yaml
pipeline:
  - step: "face_swapper"
    params:
      face_selector_mode: "reference"
      reference_face_path: "inputs/person_to_replace.jpg"
```
