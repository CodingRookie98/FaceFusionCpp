# AI 推理引擎 (AI Inference Engine)

FaceFusionCpp 依赖 **ONNX Runtime (ORT)** 和 **TensorRT** 进行高性能 AI 推理。本文档解释了我们如何管理模型和会话。

## 1. 概览

推理栈封装在 `Foundation` 层中。

*   **Wrapper**: 我们不在领域代码中直接使用 ORT C++ API，而是使用 `InferenceSession`。
*   **提供者优先级 (Provider Priority)**:
    1.  **TensorRT**: 最高性能。如果可用且支持，则优先使用。
    2.  **CUDA**: 如果 TensorRT 失败或被禁用，则作为 Nvidia GPU 的回退选项。
    3.  **CPU**: 最后的手段。非常慢。

## 2. 会话管理 (Session Management)

### 2.1 推理会话 (`InferenceSession`)
`Ort::Session` 的 RAII 包装器。
*   **初始化**: 加载模型，配置提供者和会话选项（图优化级别）。
*   **执行**: 处理输入/输出张量绑定并运行 `Run()` 方法。
*   **错误处理**: 捕获 ORT 异常并将其转换为应用程序错误。

### 2.2 会话池 (`SessionPool`)
创建 `InferenceSession` 很昂贵（特别是使用 TensorRT 时，可能会触发引擎编译）。
*   **LRU 缓存**: 我们缓存最近使用的会话。
*   **键**: 模型路径（例如 `assets/models/inswapper_128.onnx`）。
*   **策略**: 如果池已满，最近最少使用的会话将被逐出（从 GPU 卸载）。

## 3. 模型仓库 (Model Repository)

`ModelRepository` 类管理物理模型文件。

*   **基础路径**: 在 `app_config.yaml` (`models.path`) 中定义。
*   **自动下载**:
    *   如果请求的模型缺失，仓库可以从配置的 URL (HuggingFace/GitHub) 自动下载。
    *   验证: 下载后检查文件哈希 (SHA256) 以确保完整性。

## 4. TensorRT 优化

TensorRT 提供最佳性能，但需要针对 GPU 型号的特定"引擎"文件。

### 4.1 引擎缓存
*   **机制**: ORT 支持缓存编译后的 TensorRT 引擎。
*   **位置**: `.cache/tensorrt/` (可配置)。
*   **文件名**: 模型签名 + GPU 属性的哈希。

### 4.2 首次运行编译
当您首次使用 TensorRT 运行模型时：
1.  FaceFusionCpp 加载 ONNX 模型。
2.  ORT/TensorRT 编译图（此过程需要 **几分钟**）。
3.  编译后的引擎保存到磁盘。
4.  后续运行将瞬间加载缓存的引擎 (< 1秒)。

> **注意**: 更新 GPU 驱动程序或 TensorRT 版本会使缓存失效，触发重新编译。
