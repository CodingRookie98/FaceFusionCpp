# 应用层配置设计备忘录 (Application Layer Configuration Design) - V1.1 (Refined)

> **文档说明**：基于 [配置文件设计思路] 优化，采用模块化与 Pipeline 设计模式。本文档详细定义配置结构、设计原则及工程化约束。

## 1. 核心架构：关注点分离 (Separation of Concerns)

为了提高系统的可维护性与扩展性，我们将配置严格划分为**静态环境**与**动态逻辑**。

### 1.1 App Config (环境与基础设施)
*   **定位**：通过 `app_config.yaml` 定义整个应用的运行时环境。
*   **特性**：
    *   **静态 (Static)**：程序启动时加载。
    *   **全局 (Global)**：作用于整个应用生命周期。
    *   **不可变 (Immutable)**：运行时一般不修改。
    *   **模块化 (Modular)**：包含 Server、Logging、Inference、Resource 等基础设施配置。

### 1.2 Task Config (业务流水线)
*   **定位**：通过 `task_config.yaml` 或 `task_*.yaml` 定义具体的业务处理逻辑。
*   **特性**：
    *   **动态 (Dynamic)**：每次执行任务时加载。
    *   **任务级 (Task-Scoped)**：针对单次任务有效。
    *   **可变 (Mutable)**：用户可根据需求灵活调整 Pipeline。
    *   **架构模式 (Pattern)**：基于 Pipeline (Steps) 或 Actions 设计。

### 1.3 运行模式 (Execution Modes)
*   **命令行模式 (CLI)**：
    *   **当前重点**。
    *   通过命令行参数传入任务配置文件路径（e.g., `--config task.yaml`）。
*   **服务模式 (Server)**：
    *   **未来规划**。
    *   通过 HTTP/Socket 接收动态 JSON/YAML 任务配置。
*   **设计原则**：核心接口 `RunPipeline(TaskConfig)` 必须与输入源解耦，确保 CLI 和 Server 模式仅在"配置加载层"有所不同，底层逻辑完全复用。

---

## 2. 详细设计规范 (Detailed Design Specification)

### 2.1 App Config 设计 (`app_config.yaml`)

采用分层结构而非扁平结构，以增强可读性。

```yaml
# Schema Version
config_version: "1.0"

# 1. 核心应用信息
# 注意：App Name 和 Version 不在此定义，由编译期注入 (见 Sec 6.5)

# 2. 推理基础设施 (Inference Infrastructure)
inference:
  # 显卡/计算设备分配
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
  # 日志存储目录 (注意：文件名固定为 app.log 或程序指定，不可配置，仅目录可配)
  directory: "./logs"
  rotation: "daily"

# 5. 模型管理 (Model Management)
models:
  # 模型基础目录
  # 逻辑: 完整路径 = app_config.models.path + models_info.item.file_name
  path: "./assets/models"
  download_strategy: "auto" # force, skip, auto
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
  # 策略: 若为空由程序生成；若指定且冲突则拒绝的任务。
  id: "task_default_001"
  description: "Face swap and enhancement pipeline"
  # 是否启用独立任务日志 (Optional)
  # 若启用，将在日志目录生成 {task_id}.log
  enable_logging: true

# 2. 输入输出 (I/O)
io:
  # 输入源列表 (支持多源)
  source_paths:
    - "D:/projects/faceFusionCpp/data/source_face.jpg" # 强制绝对路径 (Sec 6.1)

  # 目标列表
  target_paths:
    - "D:/projects/faceFusionCpp/data/target_video.mp4" # 强制绝对路径

  # 输出配置
  output:
    path: "D:/projects/faceFusionCpp/data/output/"     # 强制绝对路径
    prefix: "result_"
    subfix: "_v1"

    # 格式配置
    image_format: "png"        # [png, jpg, bmp]
    video_encoder: "libx264"   # [libx264, libx265, h264_nvenc, ...]
    video_quality: 80          # [0-100]

# 3. 资源控制 (Resource Control)
resource:
  # 任务并发线程数 (Thread count for this specific task)
  # 0: Auto (默认为机器最大线程数的一半 / 50% of CPU Cores)
  thread_count: 0
  # 处理顺序策略:
  # sequential: 顺序模式 (默认, 省空间). Asset 1 [S1->S2] -> Asset 2 [S1->S2]
  # batch: 批处理模式 (吞吐量优先). Step 1 [A1, A2] -> Step 2 [A1, A2]
  execution_order: "sequential"

# 4. 人脸分析配置 (Shared Analysis Config)
# 若多个步骤共享检测结果，可在此统一定义
face_analysis:
  face_detector:
    models: ["yolo", "retina", "scrfd"] # 融合策略
    score_threshold: 0.5
  face_landmarker:
    model: "2dfan4"
  face_masker:
    # 多遮罩融合策略 (Mask Fusion)
    types: ["box", "occlusion", "region"]
    # Supported Regions: [skin, left-eyebrow, right-eyebrow, left-eye, right-eye, eye-glasses, left-ear, right-ear, earring, nose, mouth, upper-lip, lower-lip, neck, necklace, cloth, hair, hat]
    # Default: "all" (if not specified or empty)
    region: ["face", "eyes"] # 遮罩区域
    # 融合逻辑: 将由代码内部实现最佳遮罩计算

# 5. 处理流水线 (Processing Pipeline)
# 有序定义处理步骤
# Supported Processors & Parameters:
#
# 1. face_swapper
#    - Models: [inswapper_128, inswapper_128_fp16]
#    - Params:
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#
# 2. face_enhancer
#    - Models: [codeformer, gfpgan_1.2, gfpgan_1.3, gfpgan_1.4]
#    - Params:
#        blend_factor: 0.0 - 1.0 (default: 0.8)
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#
# 3. expression_restorer
#    - Models: [live_portrait]
#    - Params:
#        restore_factor: 0.0 - 1.0 (default: 0.8)
#        face_selector_mode: [reference, one, many] (default: many)
#        reference_face_path: "path/to/face.jpg" (required if mode=reference)
#
# 4. frame_enhancer
#    - Models: [real_hatgan_x4, real_esrgan_x2, real_esrgan_x2_fp16, real_esrgan_x4, real_esrgan_x4_fp16, real_esrgan_x8, real_esrgan_x8_fp16]
#    - Params:
#        enhance_factor: 0.0 - 1.0 (default: 0.8)

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

  - step: "expression_restorer"
    enabled: false
    params:
      model: "live_portrait"
      restore_factor: 0.8
      face_selector_mode: "many"

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
    *   遍历 `target_paths` 的每一帧/图，将计算出的参考人脸“映射”到目标人脸位置。

### 3.2 Face Enhancer (人脸增强)
*   **I/O 依赖**: 仅需 `target_paths`。**忽略** `source_paths`。
*   **逻辑**: 专注于检测 `target_paths` 媒体中的人脸区域，并对其进行超分辨率修复和细节增强。

### 3.3 Expression Restorer (表情迁移)
*   **I/O 依赖**: 必须提供 `source_paths` 和 `target_paths`。
*   **逻辑**:
    *   从 `source_paths` 中提取源人脸的表情特征。
    *   **表情复制**: 将源表情直接驱动/应用到 `target_paths` 中的目标人脸上，实现“表情克隆”或驱动效果。

### 3.4 Frame Enhancer (全帧增强)
*   **I/O 依赖**: 仅需 `target_paths`。**忽略** `source_paths`。
*   **逻辑**: 对 `target_paths` 的整个画面（背景+人物）进行超分辨率处理 (Super-Resolution)，提升视频/图片的整体画质，不局限于人脸区域。

---

## 4. 实现路线图 (Implementation Roadmap)

1.  **数据结构定义 (Schema)**
    *   定义 C++ Struct/Class 严格对应上述 YAML 结构。
2.  **解析器实现 (Parser)**
    *   引入 `yaml-cpp`。
    *   App Config: 实现层级解析。
    *   Task Config: 实现多态解析（基于 `step` 字段工厂模式创建 Processor）。
3.  **校验逻辑 (Validation)**
    *   加载时进行 Schema 校验（默认值回填、类型检查、路径存在性检查）。

---

## 5. 优势总结 (Benefits)

*   **清晰度 (Clarity)**：Pipeline 列表结构直观展示处理流，优于旧版扁平列表。
*   **灵活性 (Flexibility)**：参数 (`params`) 与步骤 (`step`) 绑定，实现高内聚。
*   **扩展性 (Extensibility)**：新增功能只需注册新的 Processor 类型，无需修改核心配置结构。
*   **健壮性 (Robustness)**：环境配置与业务逻辑物理隔离，降低误操作风险。

---

## 6. 工程化约束与最佳实践 (Engineering Constraints)

为确保代码的工业级质量与可维护性，必须遵循以下约束：

### 6.1 路径锚定规则 (Path Resolution)
*   **App Config**：所有相对路径必须相对于**程序安装根目录**。
*   **Task Config**：所有文件路径（输入源、输出目录、资源引用）必须强制使用**绝对路径**。
    *   *目的*：彻底消除 CLI 模式（CWD 可能变动）与 Server 模式下的路径歧义。

### 6.2 进度汇报解耦 (Progress Reporting)
*   **禁止直接 I/O**：核心 Pipeline 接口 `RunPipeline` 严禁直接打印日志或操作控制台。
*   **回调机制**：必须通过 `std::function<void(TaskProgress)>` 注入回调。
    *   **CLI**：在 Callback 中更新控制台进度条 (tqdm-like)。
    *   **Server**：在 Callback 中通过 WebSocket 推送状态。

### 6.3 错误处理策略 (Error Handling)
*   **人脸检测失败 (No Face Detected)**：
    *   **Sequential 模式**：打印 WARN 日志，**跳过**当前 Step，继续后续 Step（若逻辑允许）或终止当前 Asset 处理。
    *   **Batch 模式**：打印 WARN 日志，**跳过**当前帧/图片，不中断整个批次。
    *   *原则*：非致命错误不应导致整个任务崩溃。

### 6.4 配置版本控制 (Versioning)
*   **强制版本号**：所有 YAML 根节点必须包含 `config_version` 字段 (e.g., `"1.0"`)。
*   **兼容性检查**：程序启动/任务加载时必须校验版本号，不兼容（如 Major 版本只有差异）应直接报错。

### 6.5 应用元数据 (Application Metadata)
*   **编译期注入**：App Name 和 Version 不在配置文件中维护。
*   **实现方式**：
    *   CMake 生成 `config.h` / `version.h`。
    *   包含宏 `#define APP_NAME "FaceFusionCpp"` 和 `#define APP_VERSION "0.3.3"`。
*   **启动展示**：`main` 函数启动时直接读取宏打印 Banner，确保版本信息的绝对真实性。

### 6.6 优雅退出 (Graceful Shutdown)
*   **信号处理**: 捕获 `SIGINT` (Ctrl+C) / `SIGTERM`。
*   **退出策略**:
    *   **停止接收**: 立即停止 Pipeline 接收新的输入帧/图片。
    *   **等待完成**: 等待当前正在推理/处理的帧完成（避免数据损坏），设定超时强制退出。
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
### 6.9 内存流控与背压 (Memory Flow Control & Backpressure)
*   **问题**: 生产者 (解码/读取) 速度 > 消费者 (模型推理) 速度时，内存会无限膨胀。
*   **解决方案**: **有界阻塞队列 (Bounded Blocking Queue)**。
    *   **机制**: 设定队列最大容量 (e.g., 32帧)。当队列满时，强制**阻塞生产者线程**，直到消费者取走数据。
    *   **效果**: 自动平衡生产与消费速度，确保低配机器内存不溢出 (OOM)。
