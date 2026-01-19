# 应用层配置设计备忘录 (Application Layer Configuration Design) - V1.1 (Refined)

## 1. 核心架构：关注点分离

我们将配置严格划分为**静态环境 (App Config)** 与 **动态逻辑 (Task Config)**。

### 1.1 App Config (环境与基础设施)
*   **特性**: 静态、全局、启动时加载、不可变。
*   **结构**: 模块化分层 (Server, Logging, Inference, Resource)。
*   **格式**: `app_config.yaml`

### 1.2 Task Config (业务流水线)
*   **特性**: 动态、任务级、运行时加载、可变。
*   **结构**: 基于 Pipeline (Steps) 或 Actions。
*   **格式**: `task_config.yaml` 或 `task_*.yaml`

### 1.3 运行模式 (Execution Modes)
*   **命令行模式 (CLI)**: 当前重点。通过命令行参数传入任务配置文件路径（e.g., `--config task.yaml`）。
*   **服务模式 (Server)**: 未来规划。通过 HTTP/Socket 接收动态任务配置。
*   **设计原则**: 核心接口 `RunPipeline(TaskConfig)` 必须与输入源解耦，确保 CLI 和 Server 模式仅在"配置加载层"有所不同，底层逻辑完全复用。

---

## 2. 详细设计规范

### 2.1 App Config 设计 (`app_config.yaml`)

不再使用扁平结构，而是分层组织：

```yaml
# Schema Version
config_version: "1.0"

# 1. 核心应用信息 (移除)
# App Name 和 Version 不在配置文件中定义，而是由编译系统 (CMake) 注入到二进制中。
# 原因：版本是程序的固有属性，防止用户配置文件与二进制版本不一致。

# 2. 推理基础设施 (Inference Infrastructure)
inference:
  # 显卡/计算设备分配
  device_id: 0
  # 引擎缓存策略
  engine_cache:
    enable: true
    path: "./.cache/tensorrt"
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
  # 仅能设置程序日志目录，不能设置日志文件名
  directory: "./logs"
  rotation: "daily"

# 5. 模型管理 (Model Management)
models:
  # 模型基础目录 (Base Directory)
  # 结构变更: `models_info.json` 将废弃 `path` 字段，新增 `file_name` 字段
  # 逻辑: 完整路径 = app_config.models.path + models_info.item.file_name
  path: "./assets/models"
  download_strategy: "auto" # force, skip, auto
```

### 2.2 Task Config 设计 (`task_config.yaml`)

采用 **Pipeline** 模式，强调步骤 (Step)与参数 (Params) 的自包含性。

```yaml
# Schema Version
config_version: "1.0"

# 1. 任务元数据 (Task Metadata)
task_info:
  # 唯一任务标识 (Runtime Unique ID)
  # 格式: [a-zA-Z0-9_] (数字字母下划线)
  # 策略:
  #   - 若为空: 程序自动生成 (e.g., task_timestamp_random)
  #   - 若用户指定且已存在: 程序拒绝添加任务 (Reject)
  id: "task_default_001"
  description: "Face swap and enhancement pipeline"
  # 是否启用独立任务日志 (Optional)
  # 若启用，将在日志目录生成 {task_id}.log，仅包含本任务日志
  enable_logging: true

# 2. 输入输出 (I/O)
io:
  # 支持多输入源 (Source: The media to be processed/referenced)
  # Examples: Replacement Face Image, Source Video for enhancement
  source_paths:
    - "./data/source_face.jpg"

  # 目标路径 (Target: The content to be swapped/modified)
  # Examples: Target Video/Image
  target_paths:
    - "./data/target_video.mp4"
  # 输出配置
  output:
    path: "./data/output/"
    prefix: "result_"
    subfix: "_v1"
    # 图片输出配置
    # Supported: [png, jpg, bmp]
    image_format: "png"
    # 视频输出配置
    # Supported: [libx264, libx265, libvpx-vp9, h264_nvenc, hevc_nvenc]
    video_encoder: "libx264"
    # Range: [0, 100] (Higher is better quality)
    video_quality: 80

# 3. 资源控制 (Resource Control)
resource:
# 3. 资源控制 (Resource Control)
resource:
  # 任务并发线程数 (Thread count for this specific task)
  thread_count: 1
  # 处理顺序策略 (Execution Order)
  # sequential: 顺序模式 (默认). 每个媒体文件依次完成所有步骤 (Asset 1 [S1->S2], Asset 2 [S1->S2]). 省空间，低延迟。
  # batch: 批处理模式 (Stage-First). 所有媒体文件完成步骤 1 后，再进行步骤 2 (Step 1 [A1, A2], Step 2 [A1, A2]). 适合最大化单一模型吞吐量，但需要保存中间结果。
  execution_order: "sequential"

# 4. 人脸分析配置 (Shared Analysis Config)
# 若多个步骤共享检测结果，可在此统一定义
face_analysis:
  face_detector:
    # 多模型并集策略 (Fusion Strategy)
    models: ["yolo", "retina", "scrfd"]
    score_threshold: 0.5
  face_landmarker:
    model: "2dfan4"
  face_masker:
    # 多遮罩融合策略 (Mask Fusion)
    types: ["box", "occlusion", "region"]
    # Supported Regions: [skin, left-eyebrow, right-eyebrow, left-eye, right-eye, eye-glasses, left-ear, right-ear, earring, nose, mouth, upper-lip, lower-lip, neck, necklace, cloth, hair, hat]
    # Default: "all" (if not specified or empty)
    region: ["face", "eyes"]
    # 融合逻辑: 将由代码内部实现最佳遮罩计算

# 4. 处理流水线 (Processing Pipeline)
# 核心变化：使用列表定义有序步骤
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
    name: "swap_main_face" # Optional alias
    enabled: true
    params:
      model: "inswapper_128_fp16"
      face_selector_mode: "reference"
      reference_face_path: "./assets/ref_face.jpg"

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
      enhance_factor: 1.0  # (Note: previously called blend)
```

---

## 3. 实现路线图

1.  **Schema Definition**: Struct (C++) 数据结构以匹配上述 YAML。
2.  **Parser Implementation**: 使用 `yaml-cpp` 实现解析器。
    *   实现层级 App Config 解析。
    *   实现多态 Task Config 解析 (根据 `step` 字段分发创建不同的 Processor)。
3.  **Validation**: 在加载阶段加入 Schema 校验（缺省值填充、类型检查）。

## 4. 优势总结

*   **清晰度**: `pipeline` 列表让处理流程一目了然，不像旧版 `frame_processors` 只是一个名字列表。
*   **灵活性**: `params` 随 `step` 而定，不再是大杂烩。
*   **扩展性**: 新增 Processor 只需在 Pipeline 中增加一种 `step` 类型。
*   **健壮性**: `inference` 和 `logging` 等环境配置与业务逻辑分离，降低误操作风险。

---

## 5. 工程化约束与最佳实践 (Engineering Constraints)

### 5.1 路径锚定规则 (Path Resolution)
*   **App Config**: 所有相对路径必须相对于**程序安装根目录**。
*   **Task Config**: 所有文件路径（输入/输出/资源）必须强制使用**绝对路径**，以避免 CLI 和 服务模式下的路径歧义。

### 5.2 进度汇报解耦 (Progress Reporting)
*   核心接口 `RunPipeline` 不应直接打印日志或操作控制台。
*   **回调机制**: 接受 `std::function<void(TaskProgress)>` 回调。
    *   **CLI 模式**: 在回调中更新控制台进度条。
    *   **Server 模式**: 在回调中推送 WebSocket 消息。

### 5.3 错误处理与容错 (Error Handling)
*   **人脸未检测到 (No Face Detected)**:
    *   **行为**: 打印警告日志 (WARN)，并**跳过**后续处理。
    *   **Sequential 模式**: 跳过当前 Step。
    *   **Batch 模式**: 跳过当前图片/帧。

### 5.4 配置版本控制 (Versioning)
*   所有 YAML 配置文件根节点必须包含 `config_version` 字段 (e.g., `config_version: "1.0"`).
*   程序加载时必须校验版本号，不兼容则报错。

### 5.5 应用元数据 (Application Metadata)
*   **来源**: **编译期注入** (Compile-time Injection)。
*   **实现**: CMake 生成 `config.h` / `version.h`，包含 `#define APP_NAME "FaceFusionCpp"` 和 `#define APP_VERSION "0.3.3"`.
*   **启动打印**: `main` 函数启动时直接读取宏定义打印 Banner，不依赖外部文件，确保任何环境下（包括丢失配置文件时）都能正确报告版本。
