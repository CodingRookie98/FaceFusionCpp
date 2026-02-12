# 系统架构设计 (System Architecture)

## 1. 概览 (5层架构)

FaceFusionCpp 遵循 **严格的 5 层分层架构 (5-Layered Architecture)**，旨在实现高内聚低耦合。这种结构确保业务逻辑与基础设施关注点分离，使系统易于测试和维护。

```mermaid
graph TD
    subgraph "Layer 1: Application (入口与配置)"
        App[CLI / AppConfig / TaskConfig]
    end

    subgraph "Layer 2: Services (编排与管理)"
        Svc[Pipeline Runner / Shutdown Handler / Checkpoint Manager]
    end

    subgraph "Layer 3: Domain (业务逻辑)"
        Dom[Face Analysis / Image Processing / Video Handling / Model Registry]
    end

    subgraph "Layer 4: Platform (硬件抽象)"
        Plat[File System / OS APIs / Time]
    end

    subgraph "Layer 5: Foundation (核心基础设施)"
        Fdn[Logger / Error Handling / AI Inference Engine (ORT/TensorRT) / Utilities]
    end

    App --> Svc
    Svc --> Dom
    Dom --> Plat
    Plat --> Fdn
    
    %% 跨层依赖 (仅允许向下)
    App --> Fdn
    Svc --> Fdn
    Dom --> Fdn
```

### 依赖规则
*   **单向依赖**: 上层仅依赖下层。
*   **无循环**: 严禁循环依赖。
*   **无反向调用**: 下层不得直接调用上层 (必要时使用回调或观察者模式)。

---

## 2. 层级详解

### 2.1 应用层 (Application Layer - `src/app/`)
*   **职责**: 程序入口、配置解析、用户交互。
*   **关键模块**:
    *   `app.cli`: 处理命令行参数 (CLI11)。
    *   `config`: 解析 YAML 配置 (`app_config.yaml`, `task_config.yaml`)。

### 2.2 服务层 (Services Layer - `src/services/`)
*   **职责**: 编排领域对象以执行复杂的工作流。管理长运行任务的生命周期。
*   **关键模块**:
    *   `services.pipeline`: 管理处理流水线 (生产者-消费者模型)。
    *   `services.shutdown`: 处理优雅停机信号 (SIGINT)。

### 2.3 领域层 (Domain Layer - `src/domain/`)
*   **职责**: 封装核心业务规则和领域逻辑。此层知道"人脸"是什么，但不关心如何在特定操作系统上绘制它。
*   **关键模块**:
    *   `domain.face`: 人脸检测、识别和换脸算法。
    *   `domain.image`: 图像 IO 和处理工具。
    *   `domain.video`: 视频帧提取和编码。
    *   `domain.ai`: 模型仓库管理。

### 2.4 平台层 (Platform Layer - `src/platform/`)
*   **职责**: 抽象操作系统特性 (文件 IO、线程原语)。
*   **关键模块**:
    *   `platform.fs`: 文件系统操作。

### 2.5 基础层 (Foundation Layer - `src/foundation/`)
*   **职责**: 提供全系统通用的低级工具。
*   **关键模块**:
    *   `foundation.ai`: 推理引擎封装 (ONNX Runtime / TensorRT)。
    *   `foundation.infrastructure`: 日志 (spdlog)、错误处理、UUID 生成。

---

## 3. 关键设计决策

### 3.1 C++20 模块 (Modules)
我们使用 C++20 模块 (`.ixx` 接口, `.cpp` 实现) 来：
*   **加速编译**: 避免头文件解析开销。
*   **物理隔离**: `.cpp` 实现的修改不会触发依赖模块的重新编译。

### 3.2 PIMPL 惯用法 (Pointer to Implementation)
在 `Foundation` 和 `Domain` 层广泛使用，以向公共 API 隐藏实现细节 (如第三方库头文件)。
*   **收益**: ABI 稳定性及更快的构建速度。

### 3.3 工厂模式 (Factory Pattern)
处理器 (如 `FaceSwapper`, `FaceEnhancer`) 通过工厂基于配置字符串动态创建。
*   **收益**: 支持"开闭原则"。新增处理器无需修改消费端代码。

### 3.4 RAII (资源获取即初始化)
所有资源 (CUDA 内存、文件句柄、推理会话) 均由 RAII 类管理。
*   **收益**: 防止资源泄漏并确保异常安全。
*   **示例**: `InferenceSession` 在析构时自动释放 TensorRT 引擎。

### 3.5 线程安全队列与背压 (Backpressure)
流水线步骤之间使用有界线程安全队列。
*   **背压机制**: 如果消费者 (如 GPU 推理) 较慢，队列会被填满，进而阻塞生产者 (如视频解码)。这有效防止了内存溢出 (OOM) 错误。
