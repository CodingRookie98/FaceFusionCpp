# 流水线内部原理 (Pipeline Internals)

## 1. 架构

处理流水线是 FaceFusionCpp 的核心。它被设计为连接 **Processor (处理器)** 的 **Linear Chain (线性链)**，中间通过 **Queue (队列)** 连接。

```mermaid
graph LR
    Source[Video Reader] -->|帧| Q1[Input Queue]
    Q1 --> P1[Face Swapper]
    P1 -->|帧| Q2[Intermediate Queue]
    Q2 --> P2[Face Enhancer]
    P2 -->|帧| Q3[Output Queue]
    Q3 --> Sink[Video Writer]
```

### 1.1 关键组件

*   **PipelineRunner**: 编排者。它创建线程，初始化处理器，并管理生命周期。
*   **Processor**: 无状态的工作单元（例如 `FaceSwapper`）。它接收一帧，执行 AI 推理，并返回修改后的帧。
*   **Adapter**: 处理器周围的包装器，处理领域逻辑（例如，裁剪人脸，贴回）。PipelineRunner 实际上编排的是 Adapter。
*   **Queue**: 有界的线程安全阻塞队列。它提供 **Backpressure (背压)**。

## 2. 执行流程

### 2.1 顺序模式 (Sequential Mode - 默认)
在此模式下，单帧在下一帧进入之前流经整个流水线（概念上如此，尽管流水线技术允许一定的并行性）。

1.  **Reader** 解码第 1 帧。
2.  **Swapper** 处理第 1 帧。
3.  **Enhancer** 处理第 1 帧。
4.  **Writer** 编码第 1 帧。

**优点**: 低延迟，低内存占用。
**缺点**: 如果在同一 GPU 上使用不同的模型，会导致频繁的模型上下文切换。

### 2.2 背压机制 (Backpressure Mechanism)
如果 **Writer** 较慢（例如，编码 4K 视频），输出队列会被填满。
1.  **Enhancer** 尝试推送到输出队列 -> **阻塞 (BLOCKS)**。
2.  **Swapper** 尝试推送到中间队列 -> **阻塞 (BLOCKS)**。
3.  **Reader** 尝试推送到输入队列 -> **阻塞 (BLOCKS)**。

这防止了系统因将整个视频读入 RAM 而导致内存不足 (OOM)。

## 3. 停机序列 (Shutdown Sequence)

优雅停机对于避免损坏视频文件至关重要。

1.  **信号**: 用户按下 Ctrl+C 或 Reader 到达 EOF。
2.  **传播**:
    *   Reader 发送 `std::nullopt` (流结束令牌 EOS) 到 Q1。
    *   Swapper 看到 EOS，完成工作，发送 EOS 到 Q2。
    *   Enhancer 看到 EOS，完成工作，发送 EOS 到 Q3。
    *   Writer 看到 EOS，完成视频文件写入，并关闭。
3.  **超时**: 如果流水线卡住（例如死锁），看门狗定时器将强制执行 `std::exit`。
