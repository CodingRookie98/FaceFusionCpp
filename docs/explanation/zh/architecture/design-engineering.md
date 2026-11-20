# 工程化约束与最佳实践 (Engineering Constraints)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-DESIGN-ENG-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 自 [design.md](./design.md) V3.3 拆分（文档治理 P3：单文档 ≤800 行红线），内容未变更。 |

> 所属总纲：[design.md](./design.md)

---

## 5. 工程化约束与最佳实践 (Engineering Constraints)

为确保代码达到工业级交付标准，必须严格遵守以下工程规范。

### 5.1 路径解析规范 (Path Resolution Criteria)
*   **根目录定义 (Root Directory)**:
    *   默认为可执行文件所在目录的上一级 (假设结构为 `bin/` 和 `config/` 同级)。
    *   可通过环境变量 `FACEFUSION_HOME` 显式覆盖。
*   **标准目录布局 (Standard Layout)**:
    *   `bin/`: 可执行文件 (ffc)
    *   `config/`: 配置文件 (`app_config.yaml`, `task_config.yaml`)
    *   `assets/`: 模型与资源
    *   `logs/`: 运行日志
*   **路径解析原则**:
    *   **App Config**: 所有路径视为 **相对路径** (Relative to Root)。
    *   **Task Config**: 所有 I/O 路径 (Source/Target/Output) 必须强制转换为 **绝对路径 (Absolute Path)**。
        *   *Rationale*: 消除不同运行模式 (CLI/Server) 下 Current Working Directory (CWD) 不一致导致的路径歧义。

### 5.2 进度与遥测解耦 (Progress & Telemetry Decoupling)
*   **IOC 原则**: 核心 Pipeline 逻辑 **严禁** 直接操作 stdout/stderr。
*   **回调机制**: 必须通过 `std::function<void(TaskProgress)>` 注入状态回调。
    *   CLI 实现：更新 tqdm 风格控制台进度条。
    *   Server 实现：推送 WebSocket 消息。

### 5.3 错误处理与恢复 (Error Handling & Recovery)

#### 5.3.1 错误码定义 (Error Codes)
> **注意**: 错误码已实装于 `src/app/config/config_types.ixx`（`enum class ErrorCode`）。下表为完整定义（含泛型 `E100/E200/E300/E400` 与文档后补的新码 E104/E204/E205/E206）。

系统采用统一的错误码规范：`Exxx` (E + 3位数字)，按模块划分区间。

| Code          | Category    | Description                | Recommended Action     |
| :------------ | :---------- | :------------------------- | :--------------------- |
| **E100-E199** | **System**  | **系统级基础设施错误**     | **重启/人工介入**      |
| E100          | System      | 通用系统错误               | 查看日志定位           |
| E101          | Resource    | Out of Memory (OOM)        | 降低并发数或Batch Size |
| E102          | Device      | CUDA Device Not Found/Lost | 检查显卡驱动及硬件     |
| E103          | Thread      | Worker Thread Deadlock     | 重启服务               |
| E104          | Device      | GPU Context Lost           | 检查驱动/重启          |
| **E200-E299** | **Config**  | **配置与初始化错误**       | **修正配置后重启**     |
| E200          | Config      | 通用配置错误               | 查看错误消息           |
| E201          | Schema      | YAML Format Invalid        | 检查配置文件语法       |
| E202          | Value       | Parameter Out of Range     | 修正参数值             |
| E203          | Path        | Config File Not Found      | 检查路径               |
| E204          | Version     | Config Version Mismatch    | 升级配置 schema        |
| E205          | Field       | Required Field Missing     | 补充必填字段           |
| E206          | Path        | Invalid Path               | 检查路径合法性         |
| **E300-E399** | **Model**   | **模型资源错误**           | **检查Assets目录**     |
| E300          | Model       | 通用模型错误               | 查看错误消息           |
| E301          | Load        | Model Load Failed          | 检查模型文件损坏/版本  |
| E302          | Missing     | Model File Missing         | 运行下载脚本           |
| **E400-E499** | **Runtime** | **运行时/业务逻辑错误**    | **视策略(Skip/Fail)**  |
| E401          | IO          | Image Decode Failed        | 跳过坏帧               |
| E402          | IO          | Video Open Failed          | 检查视频文件           |
| E403          | Face        | No Face Detected           | 透传 (Pass-through)    |
| E404          | Face        | Face Not Aligned           | 忽略或重试             |

#### 5.3.2 错误策略
*   **非致命错误策略**:
    *   **检测失败 (No Face Detected, E403)**: 记录 WARN 日志，**透传 (Pass-through)** 当前帧，即保留原始图像直接送入输出队列，保持音画同步，严禁抛出异常或直接丢帧。
    *   **Batch 容错**: 单帧失败不影响 Batch 中其他帧的处理。
*   **致命错误**: 资源耗尽 (E101)、模型文件缺失 (E302)、I/O 权限拒绝等，应立即中断并上报。

### 5.4 版本控制 (Versioning Strategy)
*   **配置协议版本**: YAML 根节点必须包含 `config_version` (e.g., "1.0")。
*   **加载校验**: 启动时校验主版本号，不兼容版本必须明确报错拒绝加载。

### 5.5 元数据管理 (Metadata Management)
*   **编译期注入**: App Name, Version, Build Time 等信息通过 CMake 生成 `version.ixx`。
*   **单一事实来源 (SSOT)**: 严禁在配置文件中手动维护版本号，`main` 函数启动 banner 必须读取编译宏。

### 5.6 优雅停机 (Graceful Shutdown)
*   **信号处理**:
    *   **Linux/Unix**: 响应 `SIGINT` (Ctrl+C) / `SIGTERM`。
    *   **Windows**: 使用 `SetConsoleCtrlHandler` 捕获 `CTRL_C_EVENT` 和 `CTRL_CLOSE_EVENT`。
*   **停机序列**:
    1.  **停止接收**: 关闭 Input Gate，拒绝新帧进入 Pipeline。
    2.  **清空队列**: 等待 In-Flight 任务（GPU 中正在推理的帧）完成，防止数据损坏。
    3.  **资源释放**: 显式释放 GPU 上下文与句柄。
    4.  **超时强制**: 设置最大等待时间 (e.g., 5s)，超时后强制 kill。

### 5.7 资源并发与流控 (Concurrency & Flow Control)
*   **线程安全**: `ResourceManager` 必须实现线程安全，支持多线程并发访问模型实例。
*   **自适应背压 (Adaptive Backpressure)**:
    *   **基于配额 (Quota-based)**: 用户配置明确的 `resource.max_memory_usage` (e.g. 4GB)。⚠️ **WIP**: 该字段尚未实现（解析未接线），当前背压仅依赖有界队列容量。
    *   **流控机制**: 使用信号量 (`std::counting_semaphore`) 维护全局内存配额。
    *   防止生产者 (CPU Decode) 速度远大于消费者 (GPU Inference) 导致的 OOM，避免依赖 OS 动态内存查询带来的不稳定性。
*   **队列生命周期管理 (Queue Lifecycle Management)**:
    *   必须实现 **显式关闭 (Explicit Shutdown)** 机制，防止消费者线程空转或死锁。
    *   **Shutdown 语义**: 标记队列"不再接受新输入" (`push` 返回失败)，但允许继续消费剩余数据。
    *   **退出条件**: 当 `State == Shutdown` 且 `Count == 0` 时，消费者收到结束信号（如 `pop` 返回 false）。
    *   **信号传递 (Propagation)**: 前级处理器的结束信号应自动触发下一级输入队列的 Shutdown，实现流水线的多米诺式自然闭合。

### 5.8 数据序列化 (Data Serialization)
*   **技术选型**: **FlatBuffers** (Google)。
*   **关键依赖**: 本模块是 [Batch 模式](design-pipeline.md#421-流水线策略-pipeline-strategy) 高效运行的前置条件。
*   **决策依据**:
    *   **Zero-Copy**: 支持直接对内存映射文件 (mmap) 进行读取而无需反序列化解析步，对于高频 IO 的 Batch 模式至关重要。
    *   **二进制效率**: 相比 JSON (需 Base64 编码，体积膨胀 ~33%)，FlatBuffers 紧凑且无编解码 CPU 开销。
    *   **强类型模式**: 通过 `.fbs` Schema 保证 C++ 结构的内存布局稳定性。
*   **实现策略**:
    *   使用 **C++20 `std::span`** 管理二进制视图，避免不必要的内存拷贝。
    *   定义 `FramePacket` Schema，包含 `Metadata` (Dims, Timestamp) 与 `Payload` (Raw Pixel Bytes / Tensor Data)。
    *   结合 `batch_buffer_mode: disk` 使用内存映射 (Memory Mapped File) 读写，减少系统调用开销。
*   **实现状态**: ✅ **已实现**。`flatbuffers` 已列入 `vcpkg.json` 依赖，生成代码位于 `src/domain/face/schema/face_generated.h`，已在人脸数据结构序列化中实际使用。

### 5.9 断点续传 (Checkpointing)
*   **启用条件**: `task_info.enable_resume: true` 时激活。
*   **机制**: 长任务定期写入 `checkpoints/{task_id}.ckpt`（JSON 格式，由 `CheckpointManager` 实现）。
*   **Checkpoint 内容**（与实现一致，`src/services/pipeline/checkpoint_manager.ixx`）:
    *   `task_id`: 任务唯一标识。
    *   `config_hash`: 任务配置的 SHA1 哈希（一致性校验，配置变更则恢复失效）。
    *   `last_completed_frame`: 已成功处理的最后一帧索引。
    *   `total_frames`: 视频总帧数。
    *   `output_path` / `output_file_size`: 当前输出文件路径与大小（部分输出校验）。
    *   `created_at` / `updated_at` / `version`: 时间戳与 Checkpoint 格式版本。
    *   `checksum`: 序列化数据的 SHA1 校验和（完整性验证）。
*   **恢复流程**:
    1.  启动时检测 `checkpoints/{task_id}.ckpt` 是否存在。
    2.  校验 checkpoint 完整性 (checksum 验证) 与配置一致性 (config_hash)。
    3.  定位到 `last_completed_frame + 1`，跳过已处理帧。
    4.  继续执行剩余帧，追加写入输出文件。
*   **清理策略**: 任务成功完成后自动删除对应 checkpoint 文件。

### 5.10 增强日志规范 (Enhanced Logging Requirements)
为了确保系统的可观测性、调试效率及生产环境问题追踪能力，必须严格执行以下日志规范。

#### 5.10.1 日志分级策略 (Log Levels)
*   **TRACE**: 极详尽的执行路径追踪。
    *   *必选场景*: 复杂算法内部循环、大对象内存分配/释放细节、锁竞争细节。
    *   *性能影响*: 仅在开发调试时开启，生产环境禁用。
*   **DEBUG**: 关键流程节点的调试信息。
    *   *必选场景*:
        *   Pipeline 中每个 Step 的输入/输出参数（Dims, Format）。
        *   Processor 初始化参数。
        *   资源加载耗时。
*   **INFO**: 业务运行状态的关键里程碑。
    *   *必选场景*:
        *   系统启动 Banner (版本、Build Time)。
        *   配置加载摘要 (Config Summary)。
        *   任务开始/结束/耗时统计。
        *   关键硬件检测结果 (GPU Name, VRAM)。
*   **WARN**: 可恢复的非预期情况。
    *   *必选场景*:
        *   E403 未检测到人脸（透传处理）。
        *   非关键配置项回退到默认值。
        *   资源使用率接近阈值 (如 VRAM > 90%)。
*   **ERROR**: 导致当前任务中断或系统退出的错误。
    *   *必选场景*:
        *   所有 Fatal 异常捕获点。
        *   I/O 严重故障 (磁盘满、权限拒绝)。

#### 5.10.2 埋点位置要求 (Instrumentation Points)
*   **入口/出口追踪 (Entry/Exit Tracing)**:
    *   所有 `Public API` 和 `Pipeline` 关键接口必须记录 Entry/Exit 日志。
    *   格式: `[ClassName::MethodName] Enter params={...}` / `Exit result={...} duration=xxms`。
*   **异常捕获块 (Catch Blocks)**:
    *   所有 `catch` 块必须记录异常详情 (what) 及堆栈信息 (如果可能)。
    *   禁止静默吞掉异常 (Swallowing Exceptions)。
*   **性能关键路径 (Critical Path)**:
    *   Pipeline 每一帧的处理总耗时。
    *   每个 Processor 的 `process()` 方法耗时。
    *   *实现*: 建议使用 RAII 风格的 `ScopedTimer` 自动记录 DEBUG 级耗时日志。

#### 5.10.3 隐私与合规 (Privacy & Compliance)
*   **敏感数据脱敏**: 严禁在日志中明文打印用户密码、API Key 等敏感信息。
*   **图像数据**: 禁止将 Raw Pixel Data 打印到日志，仅允许打印 Tensor Shape / Metadata。

---

### 5.11 Metrics JSON Schema 参考

以下为 `metrics_{timestamp}.json` 的标准输出格式：

```json
{
  "schema_version": "1.0",
  "task_id": "task_default_001",
  "timestamp": "2026-01-30T12:34:56Z",
  "duration_ms": 125000,
  "summary": {
    "total_frames": 1500,
    "processed_frames": 1500,
    "failed_frames": 0
  },
  "step_latency": [
    {
      "step_name": "main_swap",
      "avg_ms": 32.5,
      "p50_ms": 30.0,
      "p99_ms": 45.2,
      "total_ms": 48750
    },
    {
      "step_name": "post_enhancement",
      "avg_ms": 28.1,
      "p50_ms": 27.0,
      "p99_ms": 38.5,
      "total_ms": 42150
    }
  ],
  "gpu_memory": {
    "peak_mb": 4200,
    "avg_mb": 3800,
    "samples": [
      {"timestamp_ms": 0, "usage_mb": 2000},
      {"timestamp_ms": 10000, "usage_mb": 4200},
      {"timestamp_ms": 125000, "usage_mb": 3500}
    ]
  }
}
```

---
