# 5层架构实现细节 (5-Layer Architecture Implementation)

## 1. 架构概览

 FaceFusionCpp 采用严格的单向依赖流：
 `Application -> Services -> Domain -> Platform -> Foundation`

## 2. 各层级详解

### 2.1 Layer 1: Application (`src/app/`)
- **AppConfig**: 全局环境配置（显卡 ID、日志级别）。
- **TaskConfig**: 任务级配置（Pipeline 拓扑、IO 路径）。
- **CLI**: 使用 CLI11 实现的轻量级接入层。

### 2.2 Layer 2: Services (`src/services/`)
- **PipelineRunner**: 负责生产者-消费者模型的串联。
- **ShutdownHandler**: 统一管理 `SIGINT` 等终止信号。
- **CheckpointManager**: 负责长视频处理的断点续传逻辑。

### 2.3 Layer 3: Domain (`src/domain/`)
- **FaceModelRegistry**: 管理所有 AI 模型的生命周期。
- **Image/Video Wrappers**: 针对 OpenCV 和 FFmpeg 的业务逻辑封装。
- **Processors**: 执行归一化图推理的核心业务。

### 2.4 Layer 4: Platform (`src/platform/`)
- **FileSystem (fs)**: 提供跨平台的文件操作及路径规范化。
- **Threading**: 抽象 OS 线程优先级。

### 2.5 Layer 5: Foundation (`src/foundation/`)
- **InferenceEngine**: 深度封装 TensorRT 和 ONNX Runtime。
- **Logger**: 基于 `spdlog` 的增强日志系统。
- **Utilities**: 背压信号量、UUID 发生器。

## 3. 依赖规则原则

1. **禁止跨层跳跃**: 严禁 Application 直接调用 Domain 的底层私有接口，必须通过 Services 层编排。
2. **禁止循环依赖**: 发现循环依赖时，应考虑提取公共逻辑到 Foundation 层。
3. **接口优先**: 模块间通过 `.ixx` 定义的接口进行通信，隐藏内部实现（PIMPL）。
