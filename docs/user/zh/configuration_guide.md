# 配置指南 (Configuration Guide)

FaceFusionCpp 采用灵活的 YAML 配置系统。主要的配置文件有两个：

1. **`app_config.yaml`**: 全局应用程序设置 (硬件基础、路径、日志、可观测性)。
2. **`task_config.yaml`**: 特定任务设置 (I/O 策略、流水线拓扑、算法参数)。

---

## 1. 应用程序配置 (`app_config.yaml`)

此文件通常位于 `config/` 目录下，用于定义运行时环境和基础设施。**这是对整个程序生效的全局设置**。

### 结构与参数 (小白必读)

```yaml
config_version: "0.34.0"

# --- 推理基础设施 (显卡相关的设置) ---
inference:
  device_id: 0                  # GPU 设备 ID (默认: 0，如果你只有一张独显就保持为0)
  engine_cache:
    enable: true                # 启用推理引擎缓存 (默认: true。开启后第二次运行会极大地提高启动速度)
    path: "./.cache/tensorrt"   # 缓存位置 (相对于根目录)
    max_entries: 3              # 最大缓存条数 (默认: 3。就是告诉显存最多保留几个模型的缓存)
    idle_timeout_seconds: 60    # 空闲后自动释放时间 (默认: 60秒。不用模型的时候多久清理出来显存)
  default_providers:            # 默认推理后端优先级 (默认顺序: tensorrt > cuda > cpu)
    - tensorrt
    - cuda
    - cpu

# --- 资源与性能 (怎么管理你的内存和显存) ---
resource:
  # 内存策略 (默认: "strict")
  #   strict: "用完就扔"模式。在执行换脸的那一秒才把模型加载到显存里，适合显存小的机器（比如小于 8GB 的机器）。
  #   tolerant: "常驻内存"模式。启动时就把所有东西塞进去，适合想要极致处理速度且显存很大的土豪（12GB+）。
  memory_strategy: "strict"

  # 全局内存配额 (默认: "4GB")。如果你发现程序经常因为内存爆炸崩溃，可以设一个硬性上限 (比如 "4GB", "2048MB")。
  max_memory_usage: "4GB"

# --- 日志系统 (出了问题看哪里) ---
logging:
  level: "info"                 # 日志级别，支持 trace, debug, info, warn, error (默认: "info"。)
  directory: "./logs"           # 日志存在哪里
  rotation: "daily"             # 每隔多久自动新建一个日志文件 (默认: "daily"每天)
  max_files: 7                  # 最多保留多少个旧日志 (默认: 7个)
  max_total_size: "1GB"         # 日志总大小上限 (默认: "1GB")

# --- 可观测性 (用来画性能图表的工具) ---
metrics:
  enable: true                  # 是否开启 (默认: true)
  step_latency: true            # 是否记录每步换脸耗了多少毫秒 (默认: true)
  gpu_memory: true              # 是否记录显存变化曲线 (默认: true)
  report_path: "./logs/metrics_{timestamp}.json"  # 报告存哪儿

# --- 模型管理 ---
models:
  path: "./assets/models"
  # 模型如果找不到怎么办？(默认: "auto" 自动从网上下载)
  # 其他选项: "skip" (跳过并报错，如果你网络不好可以自己手动下好放进去), "force" (不管有没有，强制重新下载一次)
  download_strategy: "auto"

temp_directory: "./temp"        # 临时文件目录

# --- 默认任务设置 (回退机制) ---
# 当你忘了在具体的换脸任务里写下面这些参数时，程序就用这里的默认值
default_task_settings:
  io:
    output:
      video_encoder: "libx264"  # 视频编码器 (默认 "libx264"，兼容性最好，绝大多数播放器都能播)
      video_quality: 80         # 视频清晰度 (默认 80。范围 0-100，越高越清晰但文件越大)
      prefix: "result_"         # 自动在生成的文件名前面加个前缀 (默认 "result_")
      suffix: ""                # 自动在生成的文件名后面加个后缀 (默认 空)
      conflict_policy: "error"  # 文件重名了怎么办 (默认 "error" 报错停止。也可选 "overwrite" 直接覆盖)
      audio_policy: "copy"      # 视频声音怎么办 (默认 "copy" 把原视频的声音复制过来。也可选 "skip" 静音)
```

---

## 2. 任务配置 (`task_config.yaml`)

此文件定义了您想要执行的**具体任务**（例如，在特定视频中进行换脸）。您可以通过 `-c/--task-config` 命令行参数传入此文件。

### 2.1 基础结构描述

* **`task_info`**: 任务元数据。支持 `enable_logging` (独立日志，默认 `false`) 和 `enable_resume` (断点续传，默认 `false`。长视频如果崩了可以接着跑)。
* **`io`**: 输入源 (`source_paths`) 与目标 (`target_paths`)，支持图片、视频和目录扫描。
* **`io.output`**:
    > [!TIP]
    > 如果下面这些参数你不写，就会自动套用上面全局配置里的 `default_task_settings` 默认值。

  * `path`: 输出目录（强制绝对路径）。
  * `prefix`: 输出文件名前缀 (默认 `result_`)。
  * `suffix`: 输出文件名后缀 (默认 空)。
  * `conflict_policy`: `overwrite` (覆盖), `rename` (重命名), `error` (报错，默认值)。
  * `audio_policy`: `copy` (保留音轨，默认值), `skip` (静音)。
* **`resource`**:
  * `thread_count`: 任务并发线程数 (默认 `0`，让程序自己决定，通常是你 CPU 框框数量的一半)。
  * `max_queue_size`: 队列最大容量，控制缓冲防显存撑爆。 (默认 `20`。如果运行时狂报 OOM，请调成 10 甚至 5)。
  * `execution_order`:
    * `sequential` (默认值): 顺着一帧一帧处理。**推荐绝大部分小白使用此模式**。
    * `batch`: 各步骤分块批处理，极大降低显存峰值。**仅适合显存极小 (<=4GB) 或硬盘空间极大的环境。**
  * `batch_buffer_mode`: `memory` (存进内存，速度快) 或 `disk` (存入硬盘以防内存爆满，速度慢但稳)。
  * `segment_duration_seconds`: 视频分段处理长度（默认 `0` 不分段。超长视频可以设置为分钟级的秒数）。

### 2.2 人脸分析 (`face_analysis`)

配置全局人脸怎么检测，脸怎么切这些最基础的事情。所有的这些算法**不影响最后脸长什么样**，只影响找得准不准。

```yaml
config_version: "1.0"

task_info:
  id: "my_first_swap"
  description: "将视频 B 中的人脸替换为 A"

io:
  source_paths:
    - "inputs/face_a.jpg"       # 来源脸的图片
  target_paths:
    - "inputs/video_b.mp4"      # 目标视频
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
    models: ["yoloface", "retinaface"] # 先用yolo找，找不到再用 retina找。
    score_threshold: 0.5        # 检测置信度 (默认 0.5。调低(例如0.3)可以找到模糊的人脸，但在树叶里可能找出假脸；调高(例如0.8)找得很准，但稍微侧脸模糊的就不换了。)
  face_landmarker:
    model: "2dfan4"             # 找人脸五官关键点的模型。 (默认 2dfan4，目前最稳的)
  face_recognizer:
    model: "arcface_w600k_r50"  # 用来比较两个人是不是同一个人的模型。
    similarity_threshold: 0.6   # 相似度阈值 (默认 0.6。当选择 reference 模式时起效。设置 0.7 及其严格，不是绝对像就不换；设置 0.4 几乎是个脸就换。)
  face_masker:
    # 遮罩融合。怎么把脸天衣无缝的贴回去，不把被遮挡的头发或者眼镜抹掉。
    types: ["box", "occlusion", "region"]
    occluder_model: "xseg"                # 把手和脸前障碍物抠出来的模型。
    parser_model: "bisenet_resnet_34"     # 把五官拆开抠出来的模型。
    region: ["skin", "nose", "mouth"]     # 默认 "all" (包含所有)。小白建议不动。
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

核心业务逻辑！这些就是具体干活的"员工"（处理器），它们由一系列 `step` 组成，**根据你写的配置顺序，一项一项把水接力送下去**。

> [!NOTE]
> 如果你在 yaml 里漏写了某个 `params` 内的设置，它们会自动使用 [全局步骤参数](#3-全局步骤参数-global_pipeline_step_params) 或各自默认值。

#### **换脸处理器** (`face_swapper`)

这是最重要的干活组件：将目标中的人脸替换为源人脸。

* `model`: 模型名称，支持 `inswapper_128` 或 `inswapper_128_fp16`。(默认 `inswapper_128_fp16`。带 `fp16` 后缀的在大部分显卡上跑得更快且画质肉眼无区别)。
* `face_selector_mode`: (默认 `many`)
  * `many`: 给画面里**所有**检测到的人脸进行换脸接力。
  * `one`: 只要画面里**最大**的那张人脸，也就是主角，后面的群演不换了。
  * `reference`: 只要和 `reference_face_path` 指定图片（这张图片就是告诉程序谁是谁，只换这个人）非常相似的人脸才换。

#### **人脸增强** (`face_enhancer`)

修复人脸区域的细节和马赛克。(因为换出来的脸只有128的网纹清晰度，这一步是变高清的刚需)。

* `model`: 模型名称，支持 `codeformer`, `gfpgan_1.2`~`1.4` 等。(默认 `gfpgan_1.4`。如果画面有极度破损，可以用 codeformer)。
* `blend_factor`: 增强后的人脸与原始人脸的混合比例 (0.0 - 1.0)。(默认 `0.8`。1.0 就是完全最高清的假人脸皮肤，0 相当于白干。设置 0.8 时，会保留 20% 原本的人脸的光影，看起来最自然)。

#### **表情还原** (`expression_restorer`)

修正换脸后死板的人脸裁切图，使其眼球对视和微表情更贴切还原最初的原画面。

* `model`: 模型名称，支持 `live_portrait`。(默认 `live_portrait`)。
* `restore_factor`: 还原比例 (0.0 - 1.0)。(默认 `0.8`。建议维持默认)。

#### **全帧增强/超分** (`frame_enhancer`)

让整张模糊的原本的老照片/老视频变清楚（包括背景）。非常吃计算资源的一步。

* `model`: 模型名称，支持各种倍率的 `real_esrgan_x4_fp16` 之类。
* `enhance_factor`: 画质提升后画面占比的强度 (默认 `1.0`)。
* **Tile 分块策略**: 大视频处理的时候自动拆给计算的，防止直接爆了四倍以后显存装不下，所以通常你不需要调整。

---

## 3. 全局步骤参数 (`global_pipeline_step_params`)

如果你要指定只给某个固定人物换脸，你不想在下面每个 `face_enhancer` 和 `face_swapper` 里来回抄一遍下面那行长目录路径对吧？那你就在这里统一定义默认的脸：

```yaml
# 这个放在 pipeline 同基层级。
global_pipeline_step_params:
  # 统一指定: 面部选择只看参考对象。
  face_selector_mode: "reference"
  # 这个图里就是那个固定倒霉对象。所有步骤都默认照着这长脸操作。
  reference_face_path: "D:/assets/my_face.jpg"
```

---

## 4. 场景配置示例

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
