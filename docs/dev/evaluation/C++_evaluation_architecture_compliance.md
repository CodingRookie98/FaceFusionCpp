# C++ 架构合规性深度复核报告 (Architecture Compliance Deep Review)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-EVAL-ARCH-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |


> **文档标识**: FACE-FUSION-ARCH-REVIEW
> **审查基准**: `docs/dev/zh/architecture/design.md` (V2.8), `docs/dev/zh/architecture/layers.md`
> **审查日期**: 2026-05-29
> **审查范围**: 全部源码 (`src/`) 177 个文件 (107 .ixx, 68 .cpp, 2 .h)
> **审查人**: hermes (AI Agent)

---

## 执行摘要 (Executive Summary)

| 维度 | 评级 | 问题数 |
| :--- | :--- | :--- |
| **5层架构依赖** | ⚠️ 部分合规 | 9 处违规 |
| **优雅停机** | ⚠️ 部分合规 | 5 处问题 |
| **背压流控** | ⚠️ 部分合规 | 3 处缺陷 |
| **队列生命周期** | ✅ 基本合规 | 2 处隐患 |
| **错误处理** | ⚠️ 部分合规 | 6 处问题 |
| **日志规范** | ✅ 基本合规 | 2 处风险 |
| **LRU 缓存** | ✅ 合规 | 2 处优化点 |
| **元数据管理 (SSOT)** | ⚠️ 部分合规 | 1 处违规 |
| **路径解析规范** | ✅ 基本合规 | 4 处硬编码 |
| **C++20 模块化** | ✅ 合规 | — |

**总体评级**: ⚠️ **部分合规** — 核心架构方向正确，但存在 11 个高/中严重性问题需要修复。

---

## 1. 5层架构依赖合规性

### 1.1 设计要求 (design.md §1.2, layers.md §3)

> *依赖单向性 (Unidirectional Dependency)*: 上层仅依赖下层，严禁反向依赖或跨层跳跃调用。
> *禁止跨层跳跃*: 严禁 Application 直接调用 Domain 的底层私有接口，必须通过 Services 层编排。

### 1.2 审查结果

**结论**: ❌ **存在 9 处跨层依赖违规**

`src/app/cli/app_cli.cpp` 直接访问了 Domain 层和 Foundation 层的接口，违反了分层原则：

| # | 行号 | 违规代码 | 严重性 | 说明 |
| :-- | :--- | :------- | :----- | :--- |
| 1 | L24 | `import domain.ai.model_repository;` | 🔴 高 | App 层直接导入 Domain 模块 |
| 2 | L25 | `import domain.face.model_registry;` | 🔴 高 | App 层直接导入 Domain 模块 |
| 3 | L29 | `import processor.param_registry;` | 🟡 中 | App 层直接导入 Domain 处理器模块 |
| 4 | L89 | `using namespace domain::processor;` | 🔴 高 | App 层直接进入 Domain 命名空间 |
| 5 | L91 | `ProcessorParamRegistry::instance()` | 🔴 高 | App 层直接访问 Domain 单例 |
| 6 | L193 | `ModelRepository::get_instance()` | 🔴 高 | App 层直接访问 Domain 仓库单例 |
| 7 | L199-211 | `models_info.json` 文件发现逻辑 | 🔴 高 | 应封装为 Services 层工厂 |
| 8 | L250 | `FaceModelRegistry::get_instance()->clear()` | 🔴 高 | App 层直接操作 Domain 注册表 |
| 9 | L251 | `InferenceSessionRegistry::get_instance()->clear()` | 🟡 中 | App 层直接访问 Foundation 单例 |

**建议**: 创建 `services::ModelLifecycleService` 封装模型初始化/清理逻辑，App 层仅通过 Services 层访问 Domain。

---

## 2. 优雅停机合规性

### 2.1 设计要求 (design.md §5.6)

> *停机序列*: 停止接收 → 清空队列 → 资源释放 → 超时强制。
> *Linux/Unix*: 响应 `SIGINT` (Ctrl+C) / `SIGTERM`。
> *Windows*: 使用 `SetConsoleCtrlHandler` 捕获 `CTRL_C_EVENT` 和 `CTRL_CLOSE_EVENT`。

### 2.2 审查结果

**结论**: ⚠️ **ShutdownHandler 设计良好，但集成存在 5 处问题**

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| S1 | `shutdown_handler.cpp` | L117-119, L146 | 🔴 高 | 超时后 `callback_thread` 被 `detach()` 而非 join，可能干扰后续资源释放 |
| S2 | `shutdown_handler.cpp` | L145-147 | 🔴 高 | `uninstall()` 不等待 detached 回调线程，存在 use-after-free 风险 |
| S3 | `pipeline_runner.cpp` | L124, L126-143 | 🟡 中 | `cancel()` + `wait_for_completion()` 仅依赖原子标志轮询，无法中断同步处理的帧 |
| S4 | `shutdown_handler.cpp` | L123-128 | 🟡 中 | 超时轮询精度不足（100ms sleep），可能误判超时 |
| S5 | `shutdown_handler.cpp` | L215-218 | 🟢 低 | 信号处理器中使用 `std::mutex` 和 `condition_variable`，非 async-signal-safe |

**建议**:
1. 将 `callback_thread` 改为 `std::jthread` + `request_stop()`，`uninstall()` 中强制 join
2. 改用 self-pipe trick 或 `signalfd` 实现 async-signal-safe 的信号通知

---

## 3. 背压流控合规性

### 3.1 设计要求 (design.md §5.7)

> *自适应背压 (Adaptive Backpressure)*: 基于配额 (Quota-based)，使用信号量 (`std::counting_semaphore`) 维护全局内存配额。
> 防止生产者 (CPU Decode) 速度远大于消费者 (GPU Inference) 导致的 OOM。

### 3.2 审查结果

**结论**: ⚠️ **基础背压已实现，但存在 3 处缺陷**

| # | 文件 | 严重性 | 问题描述 |
| :-- | :--- | :----- | :------- |
| B1 | `pipeline_runner.cpp` 整体 | 🔴 高 | Runner 层无独立帧队列，无法实施 runner 级别的流控（设计文档要求的 `std::counting_semaphore` 未实现） |
| B2 | `pipeline_runner.cpp` L494-503 | 🟡 中 | 处理器创建失败仅 warn，不阻断 pipeline，可能导致静默功能降级 |
| B3 | `queue.ixx` L46 | 🟢 低 | `push()` 在 shutdown 时静默丢弃数据，无返回值 |

**符合项**:
- ✅ `concurrent_queue.ixx` L41: `push()` 阻塞等待队列不满 — 基础背压
- ✅ `pipeline_impl.ixx` L35-36: 输入/输出队列都有容量限制

**建议**: 在 `PipelineRunner` 中实现基于 `std::counting_semaphore` 的全局内存配额流控，与设计文档保持一致。

---

## 4. 队列生命周期管理

### 4.1 设计要求 (design.md §5.7)

> *显式关闭 (Explicit Shutdown)*: 标记队列"不再接受新输入"，但允许继续消费剩余数据。
> *退出条件*: 当 `State == Shutdown` 且 `Count == 0` 时，消费者收到结束信号。
> *信号传递*: 前级处理器的结束信号应自动触发下一级输入队列的 Shutdown。

### 4.2 审查结果

**结论**: ✅ **基本合规，存在 2 处隐患**

**符合项**:
- ✅ `concurrent_queue.ixx` L86-93: `shutdown()` 正确设置标志并唤醒所有等待线程
- ✅ `pipeline_impl.ixx` L66-76: `stop()` 正确实现先 shutdown 队列再 join worker
- ✅ `queue.ixx` L58: shutdown 后 `pop()` 返回 `std::nullopt`

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| Q1 | `pipeline_runner.cpp` | L103-108 | 🔴 高 | `run()` 无异常保护，若 `ExecuteTask` 抛异常，`m_running` 永久为 true |
| Q2 | `concurrent_queue.ixx` | L27 | 🟡 中 | 构造函数不验证 `max_size > 0`，传入 0 会导致 `push()` 永久阻塞 |

---

## 5. 错误处理合规性

### 5.1 设计要求 (design.md §5.3)

> *E403 No Face Detected*: 记录 WARN 日志，**透传 (Pass-through)** 当前帧，严禁抛出异常或直接丢帧。
> *致命错误*: 资源耗尽 (E101)、模型文件缺失 (E302) 等，应立即中断并上报。

### 5.2 审查结果

**结论**: ⚠️ **E403 透传正确，但存在 6 处问题**

**符合项**:
- ✅ `face_analysis_processor.ixx` L67-70: 帧级 E403 正确 pass-through
- ✅ `pipeline_runner.cpp` L370-374, L400-404, L429-433, L453-457: E302 正确中断 pipeline

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| E1 | `pipeline_runner.cpp` | L501-506 | 🟡 中 | 未知处理器 step 静默跳过，返回 `ok()` 而非错误码 |
| E2 | `pipeline_runner.cpp` | L210-213 | 🟡 中 | 文件不存在误用 `E402VideoOpenFailed`，语义不准确 |
| E3 | `pipeline_runner.cpp` | L321-322 | 🟢 低 | 图片加载失败被 E403 掩盖，根因不可见 |
| E4 | `app_cli.cpp` | L354 | 🟡 中 | `std::exit(1)` 绕过 RAII 和 shutdown handlers |
| E5 | `pipeline_runner.cpp` | L188-193 | 🟢 低 | `LoadSourceEmbeddings` 失败时未清理已创建的资源 |
| E6 | `pipeline_runner.cpp` | L309-339 | 🟢 低 | 所有源图片加载失败时返回 E403，但根因是文件读取失败 |

---

## 6. 日志规范合规性

### 6.1 设计要求 (design.md §5.10)

> *日志分级*: TRACE, DEBUG, INFO, WARN, ERROR 五个级别。
> *埋点位置*: 入口/出口追踪、异常捕获块、性能关键路径。
> *隐私合规*: 严禁明文打印敏感信息。

### 6.2 审查结果

**结论**: ✅ **基本合规**

**符合项**:
- ✅ 基于 `spdlog` 实现，支持 Trace/Debug/Info/Warn/Error/Critical 六个级别
- ✅ 支持 daily/hourly/size/rotating 多种轮转策略
- ✅ `ScopedTimer` RAII 计时器用于性能关键路径

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| L1 | `logger.cpp` | L147-156 | 🟡 中 | 后台清理线程 `while(true)` + `detach()`，无退出条件，程序退出时可能产生未定义行为 |
| L2 | `logger.cpp` | L127 | 🟡 中 | `m_logger` 指针替换非原子操作，多线程读取存在竞态条件 |

---

## 7. LRU 缓存合规性

### 7.1 设计要求 (design.md §3.1)

> *LRU 缓存容量上限 (Session Cache)*: `max_entries: 3`
> *空闲超时时间 (秒)*: `idle_timeout_seconds: 60`

### 7.2 审查结果

**结论**: ✅ **合规**

**符合项**:
- ✅ `session_pool.ixx` 实现了 LRU + TTL 双重缓存策略
- ✅ `session_pool.cpp` L26-76: 手动双向链表 + `unordered_map` 实现 LRU
- ✅ `session_pool.cpp` L150-173: TTL 过期清理机制
- ✅ `session_pool.ixx` L87-93: 统计信息 (hits, misses, evictions, expirations)

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| C1 | `session_pool.cpp` | L94-131 | 🟡 中 | `get_or_create()` 在持锁期间执行 factory（ONNX 模型加载），阻塞其他线程访问缓存 |
| C2 | `session_pool.cpp` | L150-173 | 🟢 低 | TTL 过期需外部显式调用 `cleanup_expired()`，不会自动清理 |

---

## 8. 元数据管理合规性 (SSOT)

### 8.1 设计要求 (design.md §5.5)

> *单一事实来源 (SSOT)*: 严禁在配置文件中手动维护版本号，`main` 函数启动 banner 必须读取编译宏。

### 8.2 审查结果

**结论**: ⚠️ **存在 1 处违规**

**符合项**:
- ✅ `project.yaml` 存在，作为项目元数据单一来源
- ✅ `scripts/update_metadata.py` 存在，用于同步元数据
- ✅ `CMakePresets.json` 使用预设定义构建配置，未硬编码编译器标志

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| M1 | `config_types.ixx` | L270 | 🟡 中 | `kSupportedConfigVersion = "0.34.1"` 硬编码，应从 `project.yaml` 生成或读取 |

---

## 9. 路径解析规范合规性

### 9.1 设计要求 (design.md §5.1)

> *App Config*: 所有路径视为 **相对路径** (Relative to Root)。
> *Task Config*: 所有 I/O 路径必须强制转换为 **绝对路径**。

### 9.2 审查结果

**结论**: ✅ **基本合规，存在 4 处硬编码路径**

| # | 文件 | 行号 | 严重性 | 问题描述 |
| :-- | :--- | :--- | :----- | :------- |
| P1 | `app_cli.cpp` | L133 | 🟡 中 | `app_config_path = "config/app_config.yaml"` 硬编码默认路径 |
| P2 | `app_cli.cpp` | L518 | 🟡 中 | `"./output/"` 硬编码默认输出目录 |
| P3 | `app_cli.cpp` | L199 | 🟢 低 | `models_info.json` 文件名硬编码 |
| P4 | `app_cli.cpp` | L204 | 🟢 低 | `models_info.json` 备选路径硬编码 |

---

## 10. C++20 模块化合规性

### 10.1 设计要求 (AGENTS.md)

> *强制 C++20*。使用模块化（`.ixx`/`.cppm` 接口，`.cpp` 实现）替代传统头文件。

### 10.2 审查结果

**结论**: ✅ **合规**

| 指标 | 数值 | 说明 |
| :--- | :--- | :--- |
| `.ixx` 接口文件 | 107 | 占源文件总数 60% |
| `.cpp` 实现文件 | 68 | 占源文件总数 38% |
| `.h` 传统头文件 | 2 | 仅 `ffmpeg_raii.h` 和 `face_generated.h`（FlatBuffers 生成） |
| 模块化率 | **98.9%** | 175/177 文件使用 C++20 模块 |

**符合项**:
- ✅ CMakeLists.txt 使用 `FILE_SET CXX_MODULES` 声明模块
- ✅ 接口与实现分离（`.ixx` 接口，`.cpp` 实现）
- ✅ 仅有的 2 个 `.h` 文件有合理理由（FFmpeg C API 封装、FlatBuffers 生成代码）

---

## 问题汇总 (Consolidated Findings)

### 按严重性统计

| 严重性 | 数量 | 占比 |
| :----- | :--- | :--- |
| 🔴 高 | 11 | 32% |
| 🟡 中 | 16 | 47% |
| 🟢 低 | 7 | 21% |
| **总计** | **34** | 100% |

### 按类别统计

| 类别 | 🔴 高 | 🟡 中 | 🟢 低 | 合计 |
| :--- | :--- | :--- | :--- | :--- |
| 5层架构依赖 | 7 | 2 | 0 | 9 |
| 优雅停机 | 2 | 2 | 1 | 5 |
| 背压流控 | 1 | 1 | 1 | 3 |
| 队列生命周期 | 1 | 1 | 0 | 2 |
| 错误处理 | 0 | 3 | 3 | 6 |
| 日志规范 | 0 | 2 | 0 | 2 |
| LRU 缓存 | 0 | 1 | 1 | 2 |
| 元数据管理 | 0 | 1 | 0 | 1 |
| 路径解析 | 0 | 2 | 2 | 4 |

---

## 修复优先级建议

### P0 — 立即修复（影响正确性/安全性）

1. **S1/S2**: 将 `callback_thread` 改为 `std::jthread`，`uninstall()` 中强制 join
2. **Q1**: 在 `run()` 中使用 RAII guard 确保 `m_running = false` 在任何退出路径执行
3. **E4**: 将 `std::exit(1)` 改为 `throw` 或 error return

### P1 — 短期修复（影响架构合规性）

4. **依赖违规 (9处)**: 创建 `services::ModelLifecycleService` 封装模型初始化/清理
5. **E1**: `AddProcessorsToPipeline` 未知 step 应返回错误而非 warn+忽略
6. **B1**: 在 Runner 层实现基于 `std::counting_semaphore` 的全局内存配额

### P2 — 中期优化（改善健壮性）

7. **E5/E6**: 改用 self-pipe trick 实现 async-signal-safe 信号通知
8. **C1**: `SessionPool::get_or_create()` 采用 double-checked locking 减少锁粒度
9. **L1**: Logger 后台线程改为可 join 的线程，析构时等待退出
10. **M1**: `kSupportedConfigVersion` 从 `project.yaml` 自动生成

### P3 — 长期改进（改善可维护性）

11. **P1-P4**: 将硬编码路径提取到常量模块
12. **B3**: `push()` 在 shutdown 时返回 `bool` 通知调用者
13. **C2**: SessionPool TTL 过期改为后台自动清理

---

## 附录：审查方法论

本报告基于以下方法进行审查：

1. **静态代码分析**: 搜索文件内容，检查 import/include 依赖关系
2. **架构合规检查**: 对照 design.md 和 layers.md 的约束条款逐项验证
3. **并发安全审查**: 检查原子操作、锁使用、线程生命周期管理
4. **错误路径追踪**: 追踪错误从产生到处理的完整路径

审查覆盖了 `src/` 目录下全部 177 个源文件，重点关注：
- `src/app/cli/app_cli.cpp` (CLI 入口)
- `src/services/pipeline/pipeline_runner.cpp` (流水线运行器)
- `src/services/pipeline/shutdown_handler.cpp` (停机处理器)
- `src/foundation/infrastructure/concurrent_queue.ixx` (并发队列)
- `src/foundation/infrastructure/logger.cpp` (日志系统)
- `src/foundation/ai/session_pool.cpp` (会话池/LRU 缓存)
