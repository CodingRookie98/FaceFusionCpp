# 硬件指南 (Hardware Guide)

FaceFusionCpp 是一个高性能应用程序，其速度和能力很大程度上取决于您的硬件，特别是显卡 (GPU)。本指南帮助您根据硬件配置选择合适的设置。

---

## 1. 显卡分级 (GPU Tiers)

我们根据显存 (VRAM) 和计算能力将硬件分为四个等级。

### 等级 1: 旗舰级 (>= 12GB VRAM)
*   **代表硬件**: RTX 4090, RTX 3090, RTX 4080 (16GB).
*   **能力**:
    *   实时 4K 视频处理。
    *   复杂流水线 (换脸 + 增强 + 超分)。
    *   并行执行多个任务。
*   **推荐设置**:
    *   `memory_strategy`: `tolerant` (常驻显存)。
    *   `execution_order`: `sequential` (低延迟)。
    *   `max_queue_size`: 30+.

### 等级 2: 主流级 (8 - 12GB VRAM)
*   **代表硬件**: RTX 4070, RTX 3080, RTX 2080 Ti.
*   **能力**:
    *   标准 1080p/2K 视频处理。
    *   换脸 + 增强流水线。
*   **推荐设置**:
    *   `memory_strategy`: `strict` (按需加载/卸载)。
    *   `execution_order`: `sequential`。
    *   `max_queue_size`: 20。

### 等级 3: 入门级 (4 - 8GB VRAM)
*   **代表硬件**: RTX 3060, RTX 4060, RTX 2060.
*   **能力**:
    *   1080p 视频处理 (可能需要批处理模式)。
    *   720p 实时处理。
*   **推荐设置**:
    *   `memory_strategy`: `strict`。
    *   `execution_order`: `batch` (先用模型A处理所有帧，再加载模型B)。
    *   `batch_buffer_mode`: `disk` (使用磁盘缓存中间帧以节省内存)。

### 等级 4: 低端配置 (< 4GB VRAM)
*   **代表硬件**: GTX 1650, GTX 1050 Ti, MX 系列.
*   **能力**:
    *   720p 或更低分辨率。
    *   不建议在不分段的情况下处理长视频。
*   **关键设置**:
    *   `memory_strategy`: `strict`。
    *   `execution_order`: `batch`。
    *   `max_queue_size`: 5-10。
    *   `segment_duration_seconds`: 5 (将视频按 5 秒切片处理以防崩溃)。

---

## 2. 显存优化策略 (Memory Optimization)

如果您遇到 **Out of Memory (OOM)** 错误，请尝试以下步骤：

1.  **启用严格模式**:
    在 `app_config.yaml` 中设置 `resource.memory_strategy: "strict"`。这强制应用在不使用时释放模型资源。

2.  **减小队列大小**:
    在 `task_config.yaml` 中降低 `resource.max_queue_size` (例如从 20 降至 5)。这减少了在 RAM/VRAM 中缓存的帧数。

3.  **切换到批处理模式**:
    设置 `resource.execution_order: "batch"`。这会让程序先用模型 A 处理整个视频，释放模型 A，再加载模型 B。确保同一时间显存中只有一个模型。

4.  **使用磁盘缓冲**:
    如果使用批处理模式，设置 `resource.batch_buffer_mode: "disk"`。这将中间帧写入 SSD 而非占用 RAM。

---

## 3. 性能期望 (Benchmarks)

基准数据基于 **RTX 4060 Laptop (8GB)**:

| 任务 | 分辨率 | 时间 / 帧率 |
| :--- | :--- | :--- |
| 图片换脸 | 512x512 | < 1s |
| 图片换脸 | 1080p | < 2s |
| 视频换脸 | 720p | > 15 FPS |
| 视频换脸 | 1080p | > 8 FPS |

> **注意**: 启用人脸增强 (GFPGAN) 将使帧率降低约 40-60%。
