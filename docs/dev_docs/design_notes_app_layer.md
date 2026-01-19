# 应用层配置设计备忘录 (Application Layer Configuration Design)

> **文档版本**: V1.5
> **文档说明**: 基于模块化与 Pipeline 设计模式，定义配置结构、设计原则及工程化约束。

---

## 1. 核心架构：关注点分离 (Separation of Concerns)

为提高系统的可维护性与扩展性，我们将配置严格划分为 **静态环境** 与 **动态逻辑**。

### 1.1 App Config (环境与基础设施)

*   **定位**: 通过 `app_config.yaml` 定义整个应用的运行时环境。
*   **特性**:
    *   **静态 (Static)**: 程序启动时加载。
    *   **全局 (Global)**: 作用于整个应用生命周期。
    *   **不可变 (Immutable)**: 运行时一般不修改。
    *   **模块化 (Modular)**: 包含 Logging、Inference、Resource、Models 等基础设施配置。

### 1.2 Task Config (业务流水线)

*   **定位**: 通过 `task_config.yaml` 或 `task_*.yaml` 定义具体的业务处理逻辑。
*   **特性**:
    *   **动态 (Dynamic)**: 每次执行任务时加载。
    *   **任务级 (Task-Scoped)**: 针对单次任务有效。
    *   **可变 (Mutable)**: 用户可根据需求灵活调整 Pipeline。
    *   **架构模式 (Pattern)**: 基于 Pipeline (Steps) 设计。

### 1.3 运行模式 (Execution Modes)

*   **命令行模式 (CLI)**:
    *   **当前重点**。
    *   通过命令行参数传入任务配置文件路径 (e.g., `--config task.yaml`)。
*   **服务模式 (Server)**:
    *   **未来规划**。
    *   通过 HTTP/Socket 接收动态 JSON/YAML 任务配置。
*   **设计原则**: 核心接口 `RunPipeline(TaskConfig)` 必须与输入源解耦，确保 CLI 和 Server 模式仅在 "配置加载层" 有所不同，底层逻辑完全复用。

---

## 2. 详细设计规范 (Detailed Design Specification)

### 2.1 App Config 设计 (`app_config.yaml`)

采用分层结构而非扁平结构，以增强可读性。

```yaml
# Schema Version
config_version: "1.0"

# 1. 核心应用信息
# 注意: App Name 和 Version 不在此定义，由编译期注入 (见 Sec 6.5)

# 2. 推理基础设施 (Inference Infrastructure)
inference:
  # 显卡/计算设备分配
  # 扩展预留: 未来可支持 device_ids: [0, 1] 实现多卡并行推理
  device_id: 0
  # 引擎缓存策略
  engine_cache:
    enable: true
    path: "./.cache/tensorrt" # 相对路径，参考 Sec 6.1
  # 默认推理后端优先级 (Multiselect & Priority)
  default_providers:
    - tensorrt
    - cuda
    - cpu

# 3. 资源与性能 (Resources & Performance)
resource:
  # 内存策略 (strict/tolerant)
  # strict: 严格模式 (On-Demand). 处理器仅在执行时创建，用完即销毁。适合低显存环境。
  # tolerant: 宽容模式 (Cached). 处理器在启动时预加载并常驻内存。适合高频任务或高显存环境。
  memory_strategy: "strict"

# 4. 日志与调试 (System Logging)
logging:
  # 支持级别: trace, debug, info, warn, error
  level: "info"
  # 日志存储目录 (注意: 文件名固定为 app.log 或程序指定，不可配置，仅目录可配)
  directory: "./logs"
  rotation: "daily"

# 5. 模型管理 (Model Management)
models:
  # 模型基础目录
  # 逻辑: 完整路径 = app_config.models.path + models_info.item.file_name
  path: "./assets/models"
  # 下载策略:
  # force: 无论模型是否存在都强制下载
  # skip: 模型不存在时跳过下载，从 model_repository 返回空路径，后续加载模型报错退出程序
  # auto: 模型不存在时自动下载 (默认)
  download_strategy: "auto"

# 6. 临时文件管理 (Temp File Management)
temp_directory: "./temp"
```

### 2.2 Task Config 设计 (`task_config.yaml`)

采用 **Pipeline** 模式，强调步骤 (Step) 与参数 (Params) 的自包含性。

```yaml
# Schema Version
config_version: "1.0"

# 1. 任务元数据 (Task Metadata)
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

# 2. 输入输出 (I/O)
io:
  # 输入源列表 (支持多源)
  # 注意: 源目前仅支持图片文件 [png, jpg, bmp]
  # 可输入文件路径或目录路径；若为目录，自动扫描并添加目录下所有支持的图片文件
  source_paths:
    - "D:/projects/faceFusionCpp/data/source_face.jpg" # 强制绝对路径 (Sec 6.1)

  # 目标列表
  # 支持图片、视频、目录混合输入
  # 可输入文件路径或目录路径；若为目录，自动扫描并添加目录下所有支持的媒体文件
  target_paths:
    - "D:/projects/faceFusionCpp/data/target_video.mp4" # 强制绝对路径

  # 输出配置
  output:
    path: "D:/projects/faceFusionCpp/data/output/" # 强制绝对路径
    prefix: "result_"
    subfix: "_v1"

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

# 3. 资源控制 (Resource Control)
resource:
  # 任务并发线程数 (Thread count for this specific task)
  # 0: Auto (默认为机器最大线程数的一半 / 50% of CPU Cores)
  thread_count: 0
  # 处理顺序策略:
  # sequential: 顺序模式 (默认, 省空间). Asset 1 [S1->S2] -> Asset 2 [S1->S2]
  # batch: 批处理模式 (吞吐量优先). Step 1 [A1, A2] -> Step 2 [A1, A2]
  execution_order: "sequential"
  # 视频分段处理 (Optional)
  # 0: 不分段，整个视频一次性处理
  # >0: 按指定秒数分段处理，最后合并输出为单个文件
  segment_duration_seconds: 0

# 4. 人脸分析配置 (Shared Analysis Config)
# 若多个步骤共享检测结果，可在此统一定义
face_analysis:
  face_detector:
    # Models: [retinaface, scrfd, yoloface]
    models: ["yoloface", "retinaface", "scrfd"] # 融合策略
    score_threshold: 0.5
  face_landmarker:
    # Models: [2dfan4, peppa_wutz, face_landmarker_68_5]
    model: "2dfan4"
  face_recognizer:
    # 人脸识别/相似度匹配 (用于 reference 模式)
    # Models: [arcface_w600k_r50]
    model: "arcface_w600k_r50"
    # 相似度阈值: 低于此值认为不是同一人，跳过处理
    # Range: [0.0, 1.0] (越高越严格)
    similarity_threshold: 0.6
  face_masker:
    # 多遮罩融合策略 (Mask Fusion)
    # Face Occluder Models: [xseg_1, xseg_2]
    # Face Parser Models: [bisenet_resnet_18, bisenet_resnet_34]
    types: ["box", "occlusion", "region"]
    # Supported Regions: [skin, left-eyebrow, right-eyebrow, left-eye, right-eye,
    #                     eye-glasses, left-ear, right-ear, earring, nose, mouth,
    #                     upper-lip, lower-lip, neck, necklace, cloth, hair, hat]
    # Default: "all" (if not specified or empty)
    region: ["face", "eyes"] # 遮罩区域
    # 融合逻辑: 将由代码内部实现最佳遮罩计算

# 5. 处理流水线 (Processing Pipeline)
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
#          - 若参考图片中无人脸，则此图/帧跳过当前 Step
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

pipeline:
  - step: "face_swapper"
    name: "swap_main_face" # Step 别名
    enabled: true
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "reference"
      reference_face_path: "D:/ref_face.jpg" # 绝对路径

  - step: "face_enhancer"
    name: "enhance_face"
    enabled: true
    params:
      model: "codeformer"
      blend_factor: 0.8
      face_selector_mode: "many"
      reference_face_path: "D:/ref_face.jpg" # 绝对路径

  - step: "expression_restorer"
    enabled: false
    params:
      model: "live_portrait"
      restore_factor: 0.8
      face_selector_mode: "many"
      reference_face_path: "D:/ref_face.jpg" # 绝对路径

  - step: "frame_enhancer"
    enabled: false
    params:
      model: "real_esrgan_x4"
      enhance_factor: 1.0
```

---

## 3. 核心业务逻辑 (Core Business Logic)

系统根据不同的 `step` 类型，定义的处理器逻辑与 I/O 交互规则如下：

### 3.1 Face Swapper (换脸)

*   **I/O 依赖**: 必须提供 `source_paths` 和 `target_paths`。
*   **Source 处理逻辑**:
    *   读取 `source_paths` 中列出的所有图片。
    *   提取每张图片中的人脸特征。
    *   **平均融合**: 计算所有源人脸的平均特征向量 (Average Embedding)，生成一个稳定的参考人脸。这能有效抵消单张图片的质量问题。
*   **Target 处理逻辑**:
    *   遍历 `target_paths` 的每一帧/图，将计算出的参考人脸 "映射" 到目标人脸位置。
*   **内置后处理 (硬编码，非用户配置)**:
    1.  **色彩趋近 (Color Matching)**: 调整替换脸部的色调/亮度，使其与目标图像的光照环境一致。
    2.  **边缘柔化 (Edge Blending)**: 对脸部边缘进行羽化处理，减少 "贴图感"，实现自然过渡。

### 3.2 Face Enhancer (人脸增强)

*   **I/O 依赖**: 仅需 `target_paths`。**忽略** `source_paths`。
*   **逻辑**: 专注于检测 `target_paths` 媒体中的人脸区域，并对其进行超分辨率修复和细节增强。
*   **内置后处理 (硬编码)**:
    *   **边缘柔化 (Edge Blending)**: 增强区域与原图背景融合，避免边缘突兀。

### 3.3 Expression Restorer (表情迁移)

*   **I/O 依赖**: 必须提供 `source_paths` 和 `target_paths`。
*   **逻辑**:
    *   从 `source_paths` 中提取源人脸的表情特征。
    *   **表情复制**: 将源表情直接驱动/应用到 `target_paths` 中的目标人脸上，实现 "表情克隆" 或驱动效果。
*   **内置后处理 (硬编码)**:
    *   **边缘柔化 (Edge Blending)**: 变形后的脸部边缘与原图自然过渡。

### 3.4 Frame Enhancer (全帧增强)

*   **I/O 依赖**: 仅需 `target_paths`。**忽略** `source_paths`。
*   **逻辑**: 对 `target_paths` 的整个画面 (背景+人物) 进行超分辨率处理 (Super-Resolution)，提升视频/图片的整体画质，不局限于人脸区域。

---

## 4. 实现路线图 (Implementation Roadmap)

1.  **数据结构定义 (Schema)**
    *   定义 C++ Struct/Class 严格对应上述 YAML 结构。
2.  **解析器实现 (Parser)**
    *   引入 `yaml-cpp`。
    *   App Config: 实现层级解析。
    *   Task Config: 实现多态解析 (基于 `step` 字段工厂模式创建 Processor)。
3.  **校验逻辑 (Validation)**
    *   加载时进行 Schema 校验 (默认值回填、类型检查、路径存在性检查)。

---

## 5. 优势总结 (Benefits)

*   **清晰度 (Clarity)**: Pipeline 列表结构直观展示处理流，优于旧版扁平列表。
*   **灵活性 (Flexibility)**: 参数 (`params`) 与步骤 (`step`) 绑定，实现高内聚。
*   **扩展性 (Extensibility)**: 新增功能只需注册新的 Processor 类型，无需修改核心配置结构。
*   **健壮性 (Robustness)**: 环境配置与业务逻辑物理隔离，降低误操作风险。

---

## 6. 工程化约束与最佳实践 (Engineering Constraints)

为确保代码的工业级质量与可维护性，必须遵循以下约束：

### 6.1 路径锚定规则 (Path Resolution)

*   **App Config**: 所有相对路径必须相对于 **程序安装根目录**。
*   **Task Config**: 所有文件路径 (输入源、输出目录、资源引用) 必须强制使用 **绝对路径**。
    *   *目的*: 彻底消除 CLI 模式 (CWD 可能变动) 与 Server 模式下的路径歧义。

### 6.2 进度汇报解耦 (Progress Reporting)

*   **禁止直接 I/O**: 核心 Pipeline 接口 `RunPipeline` 严禁直接打印日志或操作控制台。
*   **回调机制**: 必须通过 `std::function<void(TaskProgress)>` 注入回调。
    *   **CLI**: 在 Callback 中更新控制台进度条 (tqdm-like)。
    *   **Server**: 在 Callback 中通过 WebSocket 推送状态。

### 6.3 错误处理策略 (Error Handling)

*   **人脸检测失败 (No Face Detected)**:
    *   **Sequential 模式**: 打印 WARN 日志，**跳过当前 Step**，继续执行后续 Step (若存在)。
    *   **Batch 模式**: 打印 WARN 日志，**跳过当前帧/图片**，不中断整个批次。
    *   *原则*: 非致命错误不应导致整个任务崩溃。

### 6.4 配置版本控制 (Versioning)

*   **强制版本号**: 所有 YAML 根节点必须包含 `config_version` 字段 (e.g., `"1.0"`)。
*   **兼容性检查**: 程序启动/任务加载时必须校验版本号，不兼容 (如 Major 版本差异) 应直接报错。

### 6.5 应用元数据 (Application Metadata)

*   **编译期注入**: App Name、Version、描述、LICENSE 等信息不在配置文件中维护。
*   **实现方式**:
    *   CMake 生成 `config.h` / `version.h`。
    *   包含以下宏定义：
        *   `#define APP_NAME "FaceFusionCpp"`
        *   `#define APP_VERSION "x.x.x"`
        *   `#define APP_DESCRIPTION "High-performance face processing toolkit"`
        *   `#define APP_LICENSE "GPL-3.0"`
        *   `#define APP_GITHUB_URL "https://github.com/CodingRookie98/faceFusionCpp"`
*   **启动展示**: `main` 函数启动时直接读取宏打印 Banner，确保版本信息的绝对真实性。

### 6.6 优雅退出 (Graceful Shutdown)

*   **信号处理**: 捕获 `SIGINT` (Ctrl+C) / `SIGTERM`。
*   **退出策略**:
    *   **停止接收**: 立即停止 Pipeline 接收新的输入帧/图片。
    *   **等待完成**: 等待当前正在推理/处理的帧完成 (避免数据损坏)，设定超时强制退出。
    *   **资源释放**: 有序释放显存和句柄。

### 6.7 试运行模式 (Dry-Run Mode)

*   **CLI 参数**: 支持 `--dry-run` 标志。
*   **行为**:
    *   加载并校验 App/Task 配置文件格式。
    *   检查所有输入/输出路径是否存在/可写。
    *   检查所需模型文件是否完备。
    *   **不执行**: 不加载模型到显存，不执行实际推理，仅打印 Pipeline 执行计划。

### 6.8 资源并发安全 (Concurrency Safety)

*   **目标**: 即使当前单线程，底层设计必须预留多线程/多任务并发支持。
*   **约束**: `ResourceManager` (模型加载、显存管理) 自底向上必须是 **线程安全 (Thread-Safe)** 的，建议使用读写锁 (Shared Mutex) 或原子操作保护共享资源。

### 6.9 内存流控与背压 (Memory Flow Control & Backpressure)

*   **问题**: 生产者 (解码/读取) 速度 > 消费者 (模型推理) 速度时，内存会无限膨胀。
*   **解决方案**: **自适应有界队列 (Adaptive Bounded Queue)**。
    *   **核心思想**: 队列上限不是固定值，而是根据 **当前可用系统内存** 动态计算。
    *   **计算公式**:
        ```
        安全阈值 = 总内存 × 5% (硬编码，待用户反馈后调整)
        队列最大容量 = min(硬上限, (可用内存 - 安全阈值) / 单帧内存占用)
        ```
    *   **实现要点**:
        1.  **定期采样**: 每隔 N 帧 (如每 10 帧) 查询一次系统可用内存。
        2.  **动态调整**: 内存充足时放宽上限；内存紧张时收紧上限，触发背压。
        3.  **硬性兜底**: 设置绝对上限 (如 64 帧)，防止异常情况下无限膨胀。
    *   **效果**: 自动平衡生产与消费速度，在高配机器上榨干性能，在低配机器上确保不溢出 (OOM)。

*   **平台抽象层 (Platform Abstraction)**:
    *   C++ 标准库 **不提供** 系统可用内存查询 API。
    *   **推荐自行封装** (代码量小，零依赖):
        *   Windows: `GlobalMemoryStatusEx()` → `ullAvailPhys`
        *   Linux: `sysinfo()` → `freeram * mem_unit`
    *   CMake 条件编译选择对应平台实现文件即可。

### 6.10 断点续处理 (Resume/Checkpoint)

*   **场景**: 长视频任务处理中途崩溃或被强制关闭。
*   **机制**:
    *   **进度持久化**: 定期将进度写入 `{task_id}.checkpoint` 文件。
    *   **恢复逻辑**: 重启任务时检测 checkpoint 文件，自动跳过已处理内容。
*   **Checkpoint 数据结构**:
    *   `current_frame`: 当前已处理的帧索引。
    *   `current_segment`: 当前正在处理的分段索引 (仅分段模式下有效，否则为空)。
    *   `completed_segments`: 已完成分段列表，含临时文件路径 (仅分段模式下有效)。
*   **恢复策略**:
    1.  **非分段模式**: 从 `current_frame` 继续处理。
    2.  **分段模式**: 跳过 `completed_segments`，从 `current_segment` 的 `current_frame` 继续，最后合并。
*   **配置项** (Task Config): `task_info.enable_resume: true` (默认 false)。

### 6.11 输出文件冲突策略 (Output Conflict Policy)

*   **场景**: 输出路径已存在同名文件。
*   **配置项** (Task Config): `io.output.conflict_policy`
    *   `overwrite`: 直接覆盖已存在文件。
    *   `rename`: 自动重命名 (e.g., `result_1.mp4`)。
    *   `error`: 抛出错误，拒绝执行 (默认)。

### 6.12 临时文件管理 (Temp File Management)

*   **场景**: Batch 模式下需要保存中间结果。
*   **配置项** (App Config): `temp_directory: "./temp"`
*   **清理策略**:
    *   任务 **成功完成**: 自动删除临时文件。
    *   任务 **失败/中断**: 保留临时文件以便调试。

### 6.13 任务取消机制 (Programmatic Cancellation)

*   **场景**: Server 模式下，用户通过 API 请求取消正在运行的任务。
*   **接口**: `CancelTask(task_id)` 方法。
*   **行为**: 与优雅退出 (6.6) 共享相同逻辑 (停止接收→等待当前帧→释放资源)。

### 6.14 模型预热 (Model Warm-up)

*   **场景**: 首帧推理延迟远高于后续帧 (模型首次加载到 GPU 需要时间)。
*   **机制**: Pipeline 启动时用一张空白图片进行一次 "预热推理"。
*   **目的**: 避免首帧处理时间异常长，影响进度预估准确性。

### 6.15 视频分段处理 (Video Segmentation)

*   **场景**: 处理超长视频时，希望分段输出以便于快速预览或减少单次处理内存压力。
*   **配置项** (Task Config): `resource.segment_duration_seconds`
    *   `0`: 不分段，整个视频一次性处理 (默认)。
    *   `>0`: 按指定秒数分段处理，最后合并输出为单个文件。
*   **实现要点**:
    *   **内部分段**: 按时长切分视频，每段独立处理以控制内存占用。
    *   **关键帧对齐**: 分段边界尽量与视频关键帧 (Keyframe) 对齐，避免编码异常。
    *   **断点协同**: 与断点续处理 (6.10) 协同工作，每段完成后更新 checkpoint。
    *   **临时文件**: 分段临时文件由临时文件管理 (6.12) 统一管理，合并完成后自动清理。

### 6.16 性能指标输出 (Metrics/Statistics)

*   **场景**: 用户想知道处理速度、总耗时等统计信息。
*   **行为**: 任务完成后自动输出统计摘要，包括：
    *   总帧数 / 总图片数
    *   总耗时 (秒)
    *   平均处理速度 (FPS 或 图片/秒)
*   **日志级别**: `info` 及以上时自动打印。

### 6.17 输出水印/隐写 (Watermark - 未来规划)

*   **场景**: 版权保护或内容追溯需求。
*   **规划**: 支持不可见隐写水印 (Steganography)，嵌入 task_id 或时间戳。
*   **状态**: **未实现**，作为未来扩展点。
