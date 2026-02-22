# 常见问题解答 (FAQ)

本文档解答常见的安装、运行时错误及性能相关问题。

---

## 1. 安装与启动 (Installation & Startup)

### Q: 启动提示缺少 "VCRUNTIME140.dll" 或 "MSVCP140.dll"。
**A**: 请安装 [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/zh-cn/cpp/windows/latest-supported-vc-redist) (2015-2022 版本)。

### Q: 启动提示找不到 "cudart64_12.dll" 或 "nvinfer.dll"。
**A**: 请确保已安装 CUDA Toolkit (12.x) 和 TensorRT (10.x)，并将它们的 `bin` 目录添加到系统的 PATH 环境变量中。
或者，将所需的 DLL 文件复制到 `FaceFusionCpp.exe` 所在的文件夹。

### Q: 杀毒软件报毒。
**A**: 这是一个常见的误报，因为该程序未经过数字签名。您可以将程序目录添加到杀毒软件的白名单中。

---

## 2. 运行时错误 (Runtime Errors)

FaceFusionCpp 使用特定的错误代码来标识问题。分为四类：系统 (E100-E199)、配置 (E200-E299)、模型 (E300-E399) 和 运行时 (E400-E499)。

### 系统级基础设施错误 (E100-E199)
*   **E101: 内存不足 (Out of Memory - OOM)**
    *   **原因**: GPU 显存耗尽，通常因为生产者（解码）速度远快于消费者（AI 推理）。
    *   **解决方案**:
        1.  在 `app_config.yaml` 中启用 `strict` (严格) 内存策略。
        2.  启用 **自适应背压 (Adaptive Backpressure)**: 在 `app_config.yaml` 中设置合理的 `max_memory_usage` (如 "4GB")。系统会根据此额度自动限流。
        3.  减小 `max_queue_size` (最大队列长度，建议由 20 降至 5 或 10)。
*   **E102: CUDA 设备未找到/丢失 (CUDA Device Not Found/Lost)**
    *   **原因**: 显卡驱动未安装、版本过低或硬件连接异常。
    *   **解决方案**: 运行 `./FaceFusionCpp --system-check` 进行环境完整性自检。
*   **E103: 工作线程死锁 (Worker Thread Deadlock)**
    *   **原因**: 队列资源竞争或系统资源调度异常。
    *   **解决方案**: 重启程序。如果持续出现，尝试降低 `thread_count`。

### 配置与初始化错误 (E200-E299)
*   **E201 (YAML 格式无效)**: 检查 `app_config.yaml` 或 `task_config.yaml` 语法。
*   **E202 (参数越界)**: 修正错误的参数值，如 `blend_factor` 必须在 0.0 - 1.0 之间。
*   **E203 (配置文件未找到)**: 确保指定的配置文件路径正确。

### 模型资源错误 (E300-E399)
*   **E301: 模型加载失败 (Model Load Failed)**
    *   **原因**: 模型文件损坏或版本不兼容。
*   **E302: 模型文件缺失 (Model File Missing)**
    *   **解决方案**: 确保 `models/` 目录存在并包含所需的 `.onnx` 文件。如果 `download_strategy` 设置为 `auto` 则会自动下载。

### 运行时业务逻辑错误 (E400-E499)
*   **E401 (图片解码失败) / E402 (视频打开失败)**: 检查输入文件是否损坏或格式不支持。
*   **E403 (未检测到人脸)**: 程序无法在当前帧中检测到人脸。程序会跳过此帧的处理 (保持原始画面透传)。不会导致任务失败。
*   **E404 (人脸未对齐)**: 人脸角度过大，导致关键点检测失败，将会忽略或重试。

---

## 3. 性能问题 (Performance)

### Q: 为什么第一次运行这么慢？
**A**: 首次运行时，程序需要针对您的 GPU 编译优化后的 **TensorRT 引擎**。此过程可能需要 **5-10 分钟**。
*   后续运行时将直接使用缓存的引擎，启动即用。
*   请确保在 `app_config.yaml` 中启用了 `engine_cache.enable: true`。

### Q: 为什么视频处理很慢？
**A**: 视频处理涉及解码、人脸检测 (逐帧)、换脸、增强和编码，计算量巨大。
*   **瓶颈**: 通常是人脸增强器 (GFPGAN)。禁用它可以使速度提高 2-3 倍。
*   **分辨率**: 处理 4K 视频明显慢于 1080p。

### Q: 如何加快处理速度？
1.  **禁用人脸增强** (如果不需要极致细节)。
2.  **设置合理的内存策略**: 如果显存大于 8GB，务必在 `app_config.yaml` 中开启 `memory_strategy: tolerant`。
3.  **调整线程数**: 默认 `thread_count: 0` 会使用逻辑核心数的一半。在多任务或核数极多的 CPU 上，手动设置（如 4 或 8）可能提高整体吞吐。
4.  **使用 TensorRT**: 确保 `inference.default_providers` 中 `tensorrt` 优先级最高。

---

## 4. 质量问题 (Quality)

### Q: 视频中人脸闪烁。
**A**: 这是逐帧换脸的常见问题。
*   **解决方案**: 我们正在开发时序一致性模块 (计划中)。
*   **临时方案**: 使用更高质量的源人脸，并确保目标视频中的人脸光照均匀。

### Q: 颜色看起来不对。
**A**: 在极端光照条件下，颜色匹配算法可能会失效。
*   我们目前使用 `reinhard` 或 `hist_match` 进行色彩迁移。未来版本将允许用户手动选择算法。
