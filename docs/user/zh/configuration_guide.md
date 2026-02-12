# 配置指南 (Configuration Guide)

FaceFusionCpp 采用灵活的 YAML 配置系统。主要的配置文件有两个：

1.  **`app_config.yaml`**: 全局应用程序设置 (硬件、路径、日志)。
2.  **`task_config.yaml`**: 特定任务设置 (输入、处理器、参数)。

---

## 1. 应用程序配置 (`app_config.yaml`)

此文件通常位于 `config/` 目录下，用于控制程序与系统的交互方式。

### 结构示例

```yaml
config_version: "1.0"

inference:
  device_id: 0                  # GPU 设备 ID (默认: 0)
  engine_cache:
    enable: true                # 启用 TensorRT 引擎缓存
    path: "./.cache/tensorrt"   # 缓存位置
    max_entries: 5              # 最大缓存引擎数
    idle_timeout_seconds: 60    # 缓存清理超时时间

resource:
  memory_strategy: "strict"     # 内存策略: "strict" (严格限制显存) 或 "tolerant" (宽松)

logging:
  level: "info"                 # 日志级别: trace, debug, info, warn, error
  directory: "./logs"
  rotation: "daily"             # 日志轮转: daily (每日), hourly (每时), size (大小)
  max_files: 7

models:
  path: "./assets/models"       # AI 模型文件路径
  download_strategy: "auto"     # 下载策略: auto (自动), manual (手动)

temp_directory: "./temp"
```

---

## 2. 任务配置 (`task_config.yaml`)

此文件定义了您想要执行的**具体任务**（例如，在特定视频中进行换脸）。您可以通过 `-c` 命令行参数传入此文件。

### 2.1 基础结构

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
    video_quality: 80           # 视频质量 0-100 (默认: 80)
    video_encoder: "libx264"    # FFmpeg 编码器

resource:
  thread_count: 0               # 线程数 (0 = 自动)
  execution_order: "sequential" # 执行顺序: sequential (顺序), parallel (并行)

face_analysis:
  face_detector:
    score_threshold: 0.5        # 人脸检测置信度阈值
  face_recognizer:
    similarity_threshold: 0.6   # 人脸识别相似度阈值

pipeline:
  - step: "face_swapper"
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "many" # 模式: reference (参考), many (多人), one (单人)
      reference_face_path: ""    # 如果模式为 reference，则必须提供

  - step: "face_enhancer"
    enabled: true
    params:
      model: "gfpgan_1.4"
      blend_factor: 1.0          # 混合比例 (0.0 - 1.0)
```

### 2.2 管道处理器 (Processors)

您可以在 `pipeline` 列表中串联多个处理器。

#### **Face Swapper** (`face_swapper`)
将目标中的人脸替换为源人脸。
*   `model`: 模型名称 (例如 `inswapper_128_fp16`)。
*   `face_selector_mode`:
    *   `many`: 替换所有检测到的人脸。
    *   `one`: 仅替换最大的一张人脸。
    *   `reference`: 仅替换与 `reference_face_path` 匹配的人脸。

#### **Face Enhancer** (`face_enhancer`)
修复人脸区域的细节。
*   `model`: 模型名称 (例如 `gfpgan_1.4`)。
*   `blend_factor`: 混合比例 (0.0 - 1.0)。1.0 为完全增强。

#### **Frame Enhancer** (`frame_enhancer`)
对整个画面/帧进行超分辨率处理。
*   `model`: 模型名称 (例如 `real_esrgan_x2_fp16`)。
*   `enhance_factor`: 增强强度 (0.0 - 1.0)。

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
