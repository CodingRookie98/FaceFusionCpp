# 配置体系设计 (Configuration Design)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-DESIGN-CONFIG-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 自 [design.md](./design.md) V3.3 拆分（文档治理 P3：单文档 ≤800 行红线），内容未变更。 |

> 所属总纲：[design.md](./design.md)

---

### 3.1 应用配置 (App Configuration)
采用分层架构设计，确保配置的可读性与逻辑性。
*   **默认位置**: `config/app_config.yaml` (相对于系统根目录)。
*   **配置级联**: 遵循优先级 `Task Config > User Config > System Default`。
*   **硬编码禁令**: 必须显式定义所有关键路径与资源限额，禁止在代码中硬编码环境相关路径。

**Schema 参考**:
```yaml
# Schema Version
config_version: "0.34.1"

# 推理基础设施 (Inference Infrastructure)
inference:
  # 显卡/计算设备分配
  # 扩展预留: 未来可支持 device_ids: [0, 1]
  device_id: 0
  # 引擎缓存策略
  engine_cache:
    enable: true
    path: "./.cache/tensorrt" # 相对系统根目录 (FACEFUSION_HOME)
    # LRU 缓存容量上限 (Session Cache)
    max_entries: 3
    # 空闲超时时间 (秒)，超时后自动释放
    idle_timeout_seconds: 60
  # 默认推理后端优先级
  default_providers:
    - tensorrt
    - cuda
    - cpu

# 资源与性能 (Resources & Performance)
resource:
  # 内存策略 (strict/tolerant)
  # strict: 严格模式 (On-Demand with LRU).
  #         行为: 处理器仅在执行期间(run)持有模型资源。一旦完成当前分配的任务，尝试释放底层推理引擎资源。
  #               *优化*: 引入 Session Cache / LRU 机制，保留最近使用的 Engine Context，避免频繁加载/卸载造成的性能抖动，仅在显存压力大或超时未复用时真正释放。
  #         场景: 配合 execution_order: batch 使用效果最佳，可极大降低峰值显存。
  # tolerant: 宽容模式 (Cached).
  #         行为: 所有模型在系统启动时预加载，并常驻内存/显存，直到程序退出。
  #         场景: 适合高频实时任务或显存充足环境，避免模型重复加载开销。
  # ⚠️ 级联语义: 运行时的实际内存策略**仅由 TaskConfig 的 resource.memory_strategy 决定**
  #   (见 §3.2，runner_video 据此分支)。此处 AppConfig 级字段**不参与 MergeConfigs 合并**，
  #   仅用于启动日志摘要 (log_config_summary)。若需控制任务行为，必须在 task_config.yaml 中显式配置。
  memory_strategy: "strict"
  # 全局内存配额 (用于背压流控)
  # 格式: "4GB", "2048MB" 等
  max_memory_usage: "4GB"

# 日志与调试 (System Logging)
logging:
  # 支持级别: trace, debug, info, warn, error
  level: "info"
  # 日志存储目录 (注意: 文件名固定为 app.log 或程序指定，不可配置，仅目录可配)
  directory: "./logs"
  # 轮转策略: daily, hourly, size
  rotation: "daily"
  # 最大保留文件数 (轮转后保留最近 N 个日志文件)
  max_files: 7
  # 日志总大小上限 (超出后自动删除最旧文件)
  max_total_size: "1GB"

# 可观测性 (Observability)
metrics:
  enable: true
  # 追踪每个 Step 的耗时分布 (Avg, P99)
  step_latency: true
  # 记录 GPU 显存变化曲线
  gpu_memory: true
  # GPU 显存采样间隔 (毫秒)
  gpu_sample_interval_ms: 1000
  # 输出报告文件 (json)
  # 格式参考: 见 [Metrics JSON Schema 参考](#metrics-json-schema-参考)
  report_path: "./logs/metrics_{timestamp}.json"

# 模型管理 (Model Management)
models:
  # 模型基础目录
  path: "./assets/models"
  # 下载策略:
  # force: 无论模型是否存在都强制下载
  # skip: 模型不存在时跳过下载，从 model_repository 返回空路径，后续加载模型报错退出程序
  # auto: 模型不存在时自动下载 (默认)
  download_strategy: "auto"

# 默认模型 (Default Models)
# 各处理器在未显式指定模型时的默认选择
default_models:
  face_detector: "yoloface"
  face_recognizer: "arcface_w600k_r50"
  face_swapper: "inswapper_128_fp16"
  face_enhancer: "gfpgan_1.4"
  frame_enhancer: "real_esrgan_x2_fp16"
  expression_restorer_feature: "live_portrait_feature_extractor"
  expression_restorer_motion: "live_portrait_motion_extractor"
  expression_restorer_generator: "live_portrait_generator"
  # ⚠️ WIP: 上述 3 个 expression_restorer 模型字段当前未接线为独立配置，
  #         流水线 Step 仅暴露单一 model 字段 (live_portrait)，由 runner 内部确定具体模型。

# 临时文件管理 (Temp File Management)
temp_directory: "./temp"

# 默认任务配置 (Default Task Settings)
# 若 Task Config 中未指定，则回退使用此处的默认值
# 字段名与 task_config.yaml 完全一致，只要task_config.yaml有的字段和值都可以在这里设置，这里仅展示部分
default_task_settings:
  io:
    output:
      video_encoder: "libx264"
      video_quality: 80
      prefix: "result_"      # 与 task_config 一致
      suffix: ""             # 与 task_config 一致
      conflict_policy: "error"
      audio_policy: "copy"
```

### 3.2 任务配置 (Task Configuration)

基于 **Pipeline Pattern** 设计，由有序的 **Steps** 组成。
*   **Step 自包含性**: 每个 Step 包含完整的输入参数 (`params`)，不依赖全局隐式状态。
*   **链式处理 (Chain Processing)**: 无论执行顺序 (Sequential/Batch)，流水线均为链式处理 (S1结果 -> S2输入 -> S3)，而非原始帧独立处理。

**Schema 参考**:
```yaml
# Schema Version
config_version: "1.0"

# 任务元数据 (Task Metadata)
task_info:
  # 唯一任务标识 (Runtime Unique ID)
  # 格式: [a-zA-Z0-9_]
  # 策略: 若为空由程序生成；若指定且冲突则拒绝任务。
  id: "task_default_001"
  description: "Face swap and enhancement pipeline"
  # 是否启用独立任务日志 (Optional)
  # 若启用，将在日志目录生成 {task_id}.log
  enable_logging: true
  # 是否启用断点续处理 (Optional)
  enable_resume: false

# 输入输出 (I/O)
io:
  # 输入源列表 (支持多源)
  # 注意: 源目前仅支持图片文件 [png, jpg, bmp]
  # 可输入文件路径或目录路径；若为目录，自动扫描并添加目录下所有支持的图片文件
  source_paths:
    - "D:/projects/faceFusionCpp/data/source_face.jpg"

  # 目标列表
  # 支持图片、视频、目录混合输入
  # 可输入文件路径或目录路径；若为目录，自动扫描并添加目录下所有支持的媒体文件
  target_paths:
    - "D:/projects/faceFusionCpp/data/target_video.mp4"

  # 输出配置
  output:
    path: "D:/projects/faceFusionCpp/data/output/" # 强制绝对路径
    prefix: "result_"
    suffix: "_v1"  # 注意: 拼写为 suffix

    # 格式配置
    image_format: "png"      # [png, jpg, bmp]
    video_encoder: "libx264" # [libx264, libx265, h264_nvenc, ...]
    video_quality: 80        # [0-100]
    # 输出文件冲突策略: overwrite, rename, error
    conflict_policy: "error"
    # 音频处理策略: copy, skip
    # copy: 保留原视频音轨并合并到输出 (默认)
    # skip: 跳过音频，输出静音视频
    audio_policy: "copy"

# 资源控制 (Resource Control)
resource:
  # 任务并发线程数 (Thread count for this specific task)
  # 0: Auto (默认为机器最大线程数的一半 / 50% of CPU Cores)
  thread_count: 0
  # 队列最大容量 (Max Buffer Size per Pipeline Step)
  # 控制每个步骤的输入/输出队列上限，防止内存溢出 (OOM)。
  # 策略: 实际容量 = min(max_queue_size, AvailableRAM / FrameSize)
  # 默认: 20 (1080p RGBA ~160MB/Queue; 4 queues ~640MB Total)
  max_queue_size: 20
  # 内存策略 (⚠️ 实际生效字段，见 §3.1 级联语义说明)
  # strict: 按需加载，处理器仅在执行期间持有模型资源 (配合 batch 模式可降低峰值显存)
  # tolerant: 模型预加载并常驻 (默认，适合高频/显存充足)
  memory_strategy: "tolerant"
  # 最大处理帧数 (0: 全部处理；>0: 仅处理前 N 帧，用于调试/快速验证)
  # ✅ 已接线: config_parser 已解析该字段，runner_video 按帧索引截断 (视频) / 帧计数截断 (图片)
  max_frames: 0
  # 处理顺序策略:
  # sequential: 顺序模式 (默认). 每一帧/图一次性经过流水线中所有处理器。
  #             优势: 低延迟，内存占用小 (仅需存当前帧).
  #             劣势: 频繁切换模型可能导致 VRAM 碎片或上下文切换开销 (若 strict 模式).
  # batch: 批处理模式. 全量帧 (或分块) 通过 Processor A 后再进入 Processor B.
  #        优势: 极大降低显存峰值 (可配合 strict 模式卸载模型)，最大化 GPU 吞吐量.
  #        劣势: 需要巨大的中间存储空间 (RAM/Disk)，首帧延迟高.
  #        *警告*: 处理长视频时极易导致 OOM 或磁盘爆炸。强烈建议配合 `segment_duration_seconds` 使用，或由系统自动分块 (Chunked Batch)。
  execution_order: "sequential"

  # 批处理中间存储策略 (仅 execution_order=batch 时有效)
  # memory: 存入 RAM. 速度快，但长视频易 OOM.
  # disk: 存入临时磁盘文件. 速度较慢 (IO瓶颈)，但支持无限长视频.
  # ⚠️ WIP: 该字段尚未实现 (解析未接线)，batch 模式中间结果当前仅存内存。
  batch_buffer_mode: "memory"
  # 注意:
  # 无论执行顺序如何，流水线均为链式处理 (S1结果 -> S2输入 -> S3)，而非原始帧独立处理模式。
  # 视频分段处理 (Optional)
  # 0: 不分段，整个视频一次性处理
  # >0: 按指定秒数分段处理，最后合并输出为单个文件
  # ✅ 已实现: runner_video ProcessVideoSegmented 按时间窗口分段处理，
  #   逐段 seek + 处理 + 段文件合并 (重编码拼接)，最后 Remuxer 合成音频。
  segment_duration_seconds: 0

# 人脸分析配置 (Shared Analysis Config)
# 若多个步骤共享检测结果，可在此统一定义
face_analysis:
  face_detector:
    # Models: [retinaface, scrfd, yoloface]
    models: ["yoloface", "retinaface", "scrfd"] # 融合策略: 置信度越高越优先（采用 NMS 策略进行融合）
    score_threshold: 0.5
  face_landmarker:
    # Models: [2dfan4, peppa_wutz, face_landmarker_68_5]
    model: "2dfan4"
  face_recognizer:
    # 人脸识别/相似度匹配
    # Models: [arcface_w600k_r50]
    model: "arcface_w600k_r50"
    # 相似度阈值: 低于此值认为不是同一人，跳过处理
    # Range: [0.0, 1.0] (越高越严格)
    similarity_threshold: 0.6
  face_masker:
    # 多遮罩融合策略 (Mask Fusion)
    # 遮罩类型组合 (多选)
    types: ["box", "occlusion", "region"]
    # Supported Regions: [skin, left-eyebrow, right-eyebrow, left-eye, right-eye,
    #                     eye-glasses, left-ear, right-ear, earring, nose, mouth,
    #                     upper-lip, lower-lip, neck, necklace, cloth, hair, hat]
    # 默认: ["face", "eyes"] (实现默认值，见 task_config.ixx FaceMaskerConfig)
    region: ["face", "eyes"] # 遮罩区域
    # 注意: 实现中 occluder_model / parser_model 未作为独立配置字段暴露，
    #       遮罩模型由 face_analysis 组件内部确定 (occlusion 使用 xseg，region 使用 bisenet_resnet_34)
    # 融合逻辑: 将由代码内部实现最佳遮罩计算

# 处理流水线 (Processing Pipeline)
# 有序定义处理步骤
#
# Supported Processors & Parameters:
#
# 1. face_swapper
#    - Models: [inswapper_128, inswapper_128_fp16]
#    - Params:
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#          - 只有当图/帧中检测到与此参考图片中人脸相似的人脸时，才对该相似人脸进行处理
#          - 若参考图片中无人脸，则此图/帧透传 (Pass-through)，即直接输出原始帧，不进行处理
#
# 2. face_enhancer
#    - Models: [codeformer, gfpgan_1.2, gfpgan_1.3, gfpgan_1.4]
#    - Params:
#        blend_factor: 0.0 - 1.0 (default: 0.8)
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#          - 同上：仅处理与参考人脸相似的人脸
#
# 3. expression_restorer
#    - Models: [live_portrait] (internally uses: feature_extractor, motion_extractor, generator)
#    - Params:
#        restore_factor: 0.0 - 1.0 (default: 0.8)
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#          - 同上：仅处理与参考人脸相似的人脸
#
# 4. frame_enhancer
#    - Models: [real_esrgan_x2, real_esrgan_x2_fp16, real_esrgan_x4, real_esrgan_x4_fp16,
#               real_esrgan_x8, real_esrgan_x8_fp16, real_hatgan_x4]
#    - Params:
#        enhance_factor: 0.0 - 1.0 (default: 0.8)
#
# 注意: 支持多个同类型 Step (如两个 face_enhancer)，每个 Step 的 name 和 params 可不同
# 注意：name 是可选的，如果name 不存在，则默认为 step 的类型+下划线+索引
# 注意：params 是可选的，如果params 不存在则使用全局参数或默认参数，如果全局参数也不存在，则使用默认参数

global_pipeline_step_params: # 全局 Step 参数, 仅当 Step 没有显式定义时才生效
  face_selector_mode: "many"
  reference_face_path: "D:/ref_face.jpg"

pipeline:
  - step: "face_swapper"
    name: "main_swap"
    enabled: true
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "reference"
      reference_face_path: "D:/ref_face.jpg"

  - step: "face_enhancer"
    name: "post_enhancement"
    enabled: true
    params:
      model: "codeformer"
      blend_factor: 0.8
      face_selector_mode: "many"
      reference_face_path: "D:/ref_face.jpg" # 绝对路径

  - step: "expression_restorer"
    name: "main_expression_restorer"
    enabled: false
    params:
      model: "live_portrait"
      restore_factor: 0.8
      face_selector_mode: "many"
      reference_face_path: "D:/ref_face.jpg" # 绝对路径

  - step: "frame_enhancer"
    name: "main_frame_enhancer"
    enabled: false
    params:
      model: "real_esrgan_x4"
      enhance_factor: 1.0

```

---

### 3.3 配置校验机制 (Config Validation Mechanism)

为在应用启动早期拦截配置错误，系统实现以下校验机制：

#### 3.3.1 校验时机
*   **启动时校验**: 应用启动时自动校验 `app_config.yaml`。✅ **已实装**: `load_app_config` 加载时校验 `config_version`（E204 拒绝启动）。
*   **任务提交时校验**: 收到任务请求时校验 `task_config.yaml`。✅ **已实装**: `load_task_config` 加载时校验 `config_version`（E204 拒绝任务）。
*   **CLI 显式校验**: 通过 `--validate` 参数进行离线校验（见 [3.5.3 CLI 参数](design-interfaces.md#353-参数规格)），执行完整字段校验（E202/E205/E206 等）。

#### 3.3.2 错误报告格式
校验失败时，错误消息必须包含 **问题路径 (YAML Path)** 以便快速定位：

```text
[E201] YAML Format Invalid: Line 45, Column 3 - unexpected mapping entry
[E202] Parameter Out of Range: pipeline[1].params.blend_factor = 1.5, expected range [0.0, 1.0]
[E203] Config File Not Found: config/app_config.yaml does not exist
```

#### 3.3.3 未来扩展
*   引入 JSON Schema 或类似机制进行结构与类型的声明式校验。
*   支持配置 diff 与 migration 工具。

---

### A.2 配置文件快速对照

| 文件                | 位置                     | 生命周期 | 主要用途                      |
| :------------------ | :----------------------- | :------- | :---------------------------- |
| `app_config.yaml`   | `config/app_config.yaml` | 进程级   | 全局环境配置 (日志、模型路径) |
| `task_config.yaml`  | `config/task_config.yaml` | 任务级   | 单次任务的 I/O 与 Pipeline    |
| `{task_id}.ckpt`    | `checkpoints/`           | 临时     | 断点续传状态                  |
| `metrics_{ts}.json` | `logs/`                  | 归档     | 任务执行指标报告              |
