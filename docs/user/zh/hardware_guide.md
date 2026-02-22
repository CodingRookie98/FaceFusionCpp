# 硬件指南 (Hardware Guide)

FaceFusionCpp 是一个高性能应用。本指南帮助您根据硬件规格（特别是 GPU）选择最优配置方案。

---

## 1. 显卡分级与策略

| 显存等级 | 代表 GPU | 推荐内存策略 | 推荐执行模式 |
| :--- | :--- | :--- | :--- |
| **旗舰 (12GB+)** | 4090, 3090 | `tolerant` (常驻) | `sequential` (极速) |
| **主流 (8-12GB)** | 4070, 3080 | `strict` | `sequential` |
| **入门 (4-8GB)** | 4060, 3060 | `strict` | `batch` (分步执行) |
| **低端 (<4GB)** | 1650, 1050 | `strict` | `batch` + `disk` 缓冲 |

---

## 2. 深度显存优化 (VRAM Optimization)

如果您收到 **Out of Memory (OOM)** 错误，请按以下优先级进行：

1.  **分步批处理 (`batch` 模式)**:
    在 `task_config.yaml` 中设置 `resource.execution_order: "batch"`。这将允许程序分阶段清空显存，保证同一时间 GPU 只负载一个模型。

2.  **降低并发与缓冲区**:
    设置 `max_queue_size: 5`。这会限制内存中待处理帧的数量。

3.  **全帧增强的高级优化 (Tile Processing)**:
    运行 `frame_enhancer`（如 Real-ESRGAN）时，若显存满溢，应用会自动开启 **Tile 分块模式**。您无需手动干预，但可以根据需要通过配置减小 `tile_size`（目前通过代码内部优化，未来将开放配置）。

4.  **分段视频处理**:
    对于 4GB 以下显卡，建议开启 `segment_duration_seconds: 5-10`，以防止 FFmpeg 合并超大视频时导致的临时内存占用飙升。

---

## 3. 性能基准 (RTX 4060 8GB 测试环境)

| 场景 | 任务流 | 分辨率 | 性能 |
| :--- | :--- | :--- | :--- |
| **静态图片** | 换脸 | 1080p | < 1.5s |
| **视频 (流式)** | 仅换脸 | 1080p | ~10 FPS |
| **视频 (全能)** | 换脸 + 人脸增强 | 720p | ~6 FPS |

> [!IMPORTANT]
> 开启 **FP16** 模型 (例如 `inswapper_128_fp16`) 可以显著降低显存占用并提升在 RTX 20/30/40 系列上的速度。
