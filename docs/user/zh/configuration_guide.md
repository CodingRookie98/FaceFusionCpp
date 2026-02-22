# 配置指南 (Configuration Guide)

FaceFusionCpp 采用灵活的 YAML 配置系统。主要的配置文件有两个：

1.  **`app_config.yaml`**: 全局应用程序设置 (硬件基础、路径、日志、可观测性)。
2.  **`task_config.yaml`**: 特定任务设置 (I/O 策略、流水线拓扑、算法参数)。

---

## 1. 应用程序配置 (`app_config.yaml`)

此文件通常位于 `config/` 目录下，用于定义运行时环境和基础设施。

### 结构与参数

```yaml
config_version: "0.34.0"

# 推理基础设施
inference:
  device_id: 0                  # GPU 设备 ID (默认: 0)
  engine_cache:
    enable: true                # 启用推理引擎缓存 (TensorRT)
    path: "./.cache/tensorrt"   # 缓存位置 (相对于根目录)
    max_entries: 3              # 最大缓存条数 (LRU)
    idle_timeout_seconds: 60    # 空闲后自动释放时间 (秒)
  default_providers:            # 默认推理后端优先级
    - tensorrt
    - cuda
    - cpu

# 资源与性能
resource:
  # 内存策略:
  #   strict: 仅在执行时持有资源，配合 LRU 释放，适合低显存环境
  #   tolerant: 启动时预加载，常驻显存，适合高频实时任务
  memory_strategy: "strict"
  # 全局内存配额 (用于背压流控)，例如 "4GB", "2048MB"
  max_memory_usage: "4GB"

# 日志系统
logging:
  level: "info"                 # trace, debug, info, warn, error
  directory: "./logs"
  rotation: "daily"             # daily, hourly, size
  max_files: 7                  # 保留最近 N 个文件
  max_total_size: "1GB"         # 日志总大小上限

# 可观测性 (指标追踪)
metrics:
  enable: true
  step_latency: true            # 记录每步耗时
  gpu_memory: true              # 记录显存变化曲线
  report_path: "./logs/metrics_{timestamp}.json"

# 模型管理
models:
  path: "./assets/models"
  download_strategy: "auto"     # auto (缺失时下载), skip (跳过并报错), force (强制重新下载)

# 临时文件目录
temp_directory: "./temp"

# 默认任务设置 (回退机制)
# 当 task_config.yaml 中缺失对应字段时使用此处的默认值
default_task_settings:
  io:
    output:
      video_encoder: "libx264"
      video_quality: 80
      prefix: "result_"
      suffix: ""
      conflict_policy: "error"
      audio_policy: "copy"
```

---

## 2. 任务配置 (`task_config.yaml`)

此文件定义了您想要执行的**具体任务**（例如，在特定视频中进行换脸）。您可以通过 `-c/--task-config` 命令行参数传入此文件。

### 2.1 基础结构描述

*   **`task_info`**: 任务元数据。支持 `enable_logging` (独立日志) 和 `enable_resume` (断点续传)。
*   **`io`**: 输入源 (`source_paths`) 与目标 (`target_paths`)，支持图片、视频和目录扫描。
*   **`io.output`**:
    *   `path`: 输出目录（强制绝对路径）。
    *   `prefix`: 输出文件名前缀。
    *   `suffix`: 输出文件名后缀。
    *   `conflict_policy`: `overwrite` (覆盖), `rename` (重命名), `error` (报错)。
    *   `audio_policy`: `copy` (保留音轨), `skip` (静音)。
*   **`resource`**:
    *   `thread_count`: 任务并发线程数，0 为自动（默认最大线程数的一半）。
    *   `max_queue_size`: 队列最大容量，控制缓冲防 OOM。
    *   `execution_order`:
        *   `sequential`: 帧顺序处理，低延迟。
        *   `batch`: 各步骤分块批处理，极大降低显存峰值（适合低显存设备）。
    *   `batch_buffer_mode`: `memory` 或 `disk` (存入 SSD 以支持超长视频)。
    *   `segment_duration_seconds`: 视频分段处理长度（防崩溃）。

### 2.2 人脸分析 (`face_analysis`)

配置全局人脸检测、定位和识别参数。

```yaml
config_version: "1.0"

task_info:
  id: "my_first_swap"
  description: "将视频 B 中的人脸替换为 A"

io:
  source_paths:
    - "inputs/face_a.jpg"       # 源人脸图片
  target_paths:
    - "inputs/video_b.mp4"      # 目标视频
  output:
    path: "outputs/result.mp4"  # 输出路径
    prefix: "result_"           # 文件名前缀
    suffix: "_v1"               # 文件名后缀
    video_quality: 80           # 视频质量 0-100 (默认: 80)
    video_encoder: "libx264"    # FFmpeg 编码器
    conflict_policy: "error"
    audio_policy: "copy"

resource:
  thread_count: 0               # 线程数 (0 = 自动)
  max_queue_size: 20            # 每个步骤队列最大容量
  execution_order: "sequential" # 执行顺序: sequential (顺序), batch (批处理)
  batch_buffer_mode: "memory"
  segment_duration_seconds: 30

face_analysis:
  face_detector:
    models: ["yoloface", "retinaface"] # 融合检测策略
    score_threshold: 0.5        # 人脸检测置信度阈值
  face_landmarker:
    model: "2dfan4"
  face_recognizer:
    model: "arcface_w600k_r50"
    similarity_threshold: 0.6   # 人脸识别相似度阈值
  face_masker:
    types: ["box", "occlusion", "region"] # 遮罩融合
    occluder_model: "xseg"                # 遮挡检测模型
    parser_model: "bisenet_resnet_34"     # 人脸解析模型
    region: ["skin", "nose", "mouth"]     # 指定处理区域

pipeline:
  - step: "face_swapper"
    name: "main_swap"            # 处理步骤标识符(可选)
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "many" # 模式: reference (参考), many (多人), one (单人)
      reference_face_path: ""    # 如果模式为 reference，则必须提供

  - step: "face_enhancer"
    name: "post_enhancement"     # 处理步骤标识符(可选)
    enabled: true
    params:
      model: "gfpgan_1.4"
      blend_factor: 1.0          # 混合比例 (0.0 - 1.0)
```

### 2.3 管道处理器 (`pipeline`)

由一系列 `step` 组成，按定义顺序链式执行。

#### **换脸处理器** (`face_swapper`)
将目标中的人脸替换为源人脸。
*   `model`: 模型名称，支持 `inswapper_128`, `inswapper_128_fp16`。
*   `face_selector_mode`:
    *   `many`: 替换所有检测到的人脸。
    *   `one`: 仅替换最大的一张人脸。
    *   `reference`: 仅替换与 `reference_face_path` 匹配的人脸。

#### **人脸增强** (`face_enhancer`)
修复人脸区域的细节。
*   `model`: 模型名称，支持 `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4`。
*   `blend_factor`: 混合比例 (0.0 - 1.0)。1.0 为完全增强。

#### **表情还原** (`expression_restorer`)
修正换脸后人脸裁切图的神态，使其贴合原始表情。
*   `model`: 模型名称，支持 `live_portrait`。
*   `restore_factor`: 还原比例 (0.0 - 1.0)。

#### **全帧增强/超分** (`frame_enhancer`)
对整个画面/帧进行超分辨率处理。
*   `model`: 模型名称，支持 `real_esrgan_x2`, `real_esrgan_x2_fp16`, `real_esrgan_x4`, `real_esrgan_x4_fp16`, `real_esrgan_x8`, `real_esrgan_x8_fp16`, `real_hatgan_x4`。
*   `enhance_factor`: 增强强度 (0.0 - 1.0)。
*   **Tile 分块策略**: 应用会自动根据显存情况切分瓦片，防止 4K 视频 OOM。

---

## 3. 全局步骤参数 (`global_pipeline_step_params`)

若多个 Step 使用相同的参数（如 `reference_face_path`），可在全局定义：

```yaml
global_pipeline_step_params:
  face_selector_mode: "reference"
  reference_face_path: "D:/assets/my_face.jpg"
```

---

## 3. 场景配置示例

### 场景 A: 高质量视频处理
启用换脸和增强，并设置高质量视频编码。

```yaml
io:
  output:
    video_quality: 95
pipeline:
  - step: "face_swapper"
  - step: "face_enhancer"
    params:
      blend_factor: 1.0
  - step: "frame_enhancer" # 可选: 全图超分辨率
```

### 场景 B: 低显存模式 (Low VRAM)
通过顺序执行和严格内存策略来减少显存占用。

```yaml
# 在 app_config.yaml 中
resource:
  memory_strategy: "strict"

# 在 task_config.yaml 中
resource:
  execution_order: "sequential"
  max_queue_size: 2             # 保持流水线队列较小
```

### 场景 C: 特定人脸替换
仅替换视频中特定的人脸。

```yaml
pipeline:
  - step: "face_swapper"
    params:
      face_selector_mode: "reference"
      reference_face_path: "inputs/person_to_replace.jpg"
```
