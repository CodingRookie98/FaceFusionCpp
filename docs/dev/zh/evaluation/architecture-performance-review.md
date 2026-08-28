# 推理架构与算法性能评估报告

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-EVAL-ARCH-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 评审中 (In Review)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.4.0** | 2026-08-28 | AI Agent | 王辉 | P0 优化批次验收合并（`18b6018`）：P0-1（`df805f2`）、P0-2（`87c03d3`）、P0-3（`a61112a`）、会话 P-2 容量（`ca2afeb`）全部修复；单元 296/296、集成 142/142、E2E 14/14 全绿。 |
| **V1.3.0** | 2026-08-28 | AI Agent | 王辉 | 新增 §7 推理会话与推理池设计评估（InferenceSession + SessionPool）：确认 configure/cleanup_expired/preload_session 全项目无调用点（配置化、TTL 清理、预加载均未接线）；输出 S-1~S-5（session 层）与 P-1~P-5（池层）问题清单，P-1 池锁持锁建 session 为 P0 级并行放大器。 |
| **V1.2.0** | 2026-08-28 | AI Agent | 王辉 | 补充 P2-2 多视频并行资源开销专项评估：显存（权重共享不翻倍，增量 10-20%/任务，LRU max_entries=3 单任务已超限）、内存（~100-300MB/视频线性增长）、收益面（仅 CPU-bound 场景）；新增 P2-2a 条目（SessionPool 容量超限）。 |
| **V1.1.0** | 2026-08-28 | AI Agent | 王辉 | 二轮讨论修正：P1-2 从"mask 重复推理"修正为"mask 系统链路未接线"；确认 Box/Occlusion/Region min 融合设计意图完整、接线缺失；量化若接线的质量收益与性能代价（共享 vs 不共享两方案）。 |
| **V1.0.0** | 2026-08-28 | AI Agent | 王辉 | 首次评估：基于代码与算法层面的第一性原理审查，覆盖推理管道（Pipeline/TaskManager）、会话管理（InferenceSession/SessionPool）与算法实现（检测/关键点/交换/增强/融合），输出 P0/P1/P2 三级问题清单与 ROI 排序优化建议。 |

---

## 1. 评估背景与范围

**评估对象**: FaceFusionCpp 的推理管理管道/流水线设计、任务调度并发模型、以及人脸处理算法实现。

**评估方法**:
1. **第一性原理**: 以"每帧处理的最小必要工作量"为准绳，逐帧追踪数据流与控制流，识别重复计算与冗余开销；
2. **对抗性验证**: 三个并行探索代理（推理管道/任务调度/算法层）独立取证，结论交叉核对，每个问题均附代码证据（文件:行号）；
3. **分级标准**: P0 = 每帧/每任务必现的直接吞吐损失；P1 = 默认配置下显著、特定场景放大；P2 = 次要或需记录。

**评估基准**: `dev` 分支 @ `7f1c3d2`。

---

## 2. 架构总览（实际数据流）

```
TaskManager (单 serial worker + exec_thread，任务级串行)
  └─ PipelineRunner (每任务新建；PIMPL；验证→source embedding→类型路由)
      ├─ ProcessImageBatch  ──→ 单 Pipeline，reader 主线程 + writer 线程
      └─ ProcessVideoTarget ──→ 每视频新建 Pipeline（普通/分段/Strict 三路径）
          └─ Pipeline::worker_loop (N 个 jthread = 硬件核数/2，默认 4)
              └─ pop_batch(4) → for each processor → push_to_output_ordered(保序)
                  ├─ FaceAnalysisProcessor：检测(4角度循环)→ 填充 swap/enhance/expression 输入
                  ├─ SwapperAdapter / FaceEnhancerAdapter / ExpressionAdapter / FrameEnhancerAdapter
                  └─ 底层：InferenceSessionRegistry → SessionPool(LRU, max=3) → Ort::Session(EP: TRT>CUDA>CPU)
```

### 2.1 关键代码路径索引

| 模块 | 文件 |
| :--- | :--- |
| 任务调度（队列/超时/取消/持久化） | `src/app/web/task_manager.cpp` |
| 主编排器（验证/加载/路由） | `src/services/pipeline/pipeline_runner.cpp` |
| 视频处理（普通/分段/Strict） | `src/services/pipeline/runner_video.cpp` |
| 图像/批次处理 | `src/services/pipeline/runner_image.cpp` |
| 帧流水线（worker 池 + 保序输出） | `src/domain/pipeline/impl/pipeline_impl.ixx` |
| 处理器适配器（warp→推理→mask→paste_back） | `src/domain/pipeline/pipeline_adapters.ixx` |
| 人脸分析（检测/关键点/识别编排） | `src/domain/face/analyser/face_analyser.cpp` |
| 帧缓存（整帧哈希 LRU） | `src/domain/face/face_store.cpp` |
| 换脸算法（InSwapper） | `src/domain/face/swapper/impl/inswapper.cpp` |
| 推理会话封装（ORT EP） | `src/foundation/ai/inference_session.cpp` |
| 会话池（LRU+TTL） | `src/foundation/ai/session_pool.cpp` |
| 会话注册中心（全局单例） | `src/foundation/ai/inference_session_registry.cpp` |

---

## 3. 设计亮点（合理性确认，不建议改动）

| # | 设计 | 说明 |
| :--- | :--- | :--- |
| ✅ | 分层架构 | app / domain / services / foundation 四层职责清晰 |
| ✅ | 任务级串行 | GPU-bound 场景下多任务并行无收益，单 worker 串行是**正确保守选择** |
| ✅ | 生产者-消费者帧流水线 | reader 主线程推帧、writer 独立线程消费、sequence_id + reorder buffer 保序 |
| ✅ | Session 三层缓存 | Registry(单例) → Pool(LRU+TTL) → TRT 引擎文件缓存，key 含 EP/设备/TRT 选项 |
| ✅ | 模型懒加载 | `ensure_loaded()` 首帧才加载，`MetricsDecorator` 非侵入式可观测 |
| ✅ | 持久化/超时/恢复语义 | 快照原子写、超时软取消、Queued 恢复入队 / Running 标记失败 |
| ✅ | 算法语义完整 | 模板对齐、Mask 组合（Box+Occlusion+Region）、颜色匹配复刻 facefusion |

---

## 4. P0 级问题（每帧/每任务必现，直接损失吞吐）

### P0-1 🔴 视频路径 FaceStore 缓存是纯负收益（每帧 2 次整帧哈希）

> ✅ **已修复（2026-08-28, `df805f2`）**: `FaceAnalyser` 新增 `set_face_cache_enabled()` + `Options.enable_face_cache`；`PipelineRunner` 视频路径禁用、图像批次保留。配套测试 3 例。

**位置**: `face_analyser.cpp:66,170` + `face_store.cpp:161-184`

**机理**:
- `is_contains(frame)` 与 `insert_faces(frame, faces)` **各做一次整帧 FNV1a 哈希**（`frame.total()*elemSize()` 逐字节扫描）；
- 视频帧几乎不可能重复 → 永远 miss → 每帧 2 次全帧哈希纯浪费（1080p ≈ 6.2MB×2/帧）；
- 每帧 Face 数据（含 68 点 landmarks）写入**全局单例 LRU（1000 帧）**，跨任务污染内存。

**结论**: 该缓存只对"同一图像跨任务复用"有意义，视频路径应禁用。

### P0-2 🔴 InSwapper embedding 变换 O(n²) 每次重算

> ✅ **已修复（2026-08-28, `87c03d3`）**: `swap_face` 按 embedding 相等缓存变换结果（mutex 保护），`prepare_input` 接收预变换数据。配套测试 3 例。

**位置**: `inswapper.cpp:111-118`

```cpp
for (i) for (j) sum += source_embedding[j] * m_initializer_array[j*len+i];  // 512×512
```

**机理**: 每帧每张脸执行 262,144 次乘加，但 `source_embedding` 与 `m_initializer_array` 在任务内恒定 → 结果恒定 → **应缓存**（任务级算一次）。

### P0-3 🔴 每视频重建 swapper 并重新解析 ONNX protobuf

> ✅ **已修复（2026-08-28, `a61112a`）**: 新增 `DomainServiceCache`（runner:types，按 `{step}:{model}` key 缓存类型擦除实例）；`PipelineRunner` 跨视频/批次复用 swapper/enhancer/restorer/frame_enhancer 工厂，E302 模型检查保留。配套测试 4 例。

**位置**: `pipeline_runner.cpp:352`（`domain_ctx` 局部变量，每视频新建）→ `pipeline_runner.cpp:380` `swapper->load_model()` → `inswapper.cpp:29-84` `init()`

**机理**:
- SessionPool 只缓存 ORT session；**initializer 解析（几 MB ONNX 文件 I/O + 512×512 FP16→FP32 转换）每次重复执行**；
- 多视频任务、分段模式（每段重建 pipeline）都会触发。

---

## 5. P1 级问题（默认配置下显著，特定场景放大）

### P1-1 🟠 Strict 模式（默认）`max_queue_size=4` 卡死流水线吞吐

**位置**: `runner_video.cpp:692` + `app.yaml:21`（`memory_strategy: "strict"` 是默认）

**机理**: 所有视频默认走 `ProcessVideoStrict`，帧队列上限压到 4 → reader 频繁阻塞、GPU 空闲窗口增大。

### P1-2 🟠 mask 系统（Box/Occlusion/Region）链路未接线——非"重复推理"

> **评估修正（2026-08-28 二轮）**: 初评将本条列为"换脸+增强同时启用 → occlusion/region mask 推理两次"，经用户澄清设计意图（可配置 box/occlusion/region，默认 box，配置其他项取 min 值）并深入核查代码后**修正**：当前代码中 occlusion/region 分割**从未执行**，问题是**链路未接线**而非重复推理。

**位置**: `face_types.ixx:70`（默认 `{Box}`）、`face_analysis_processor.ixx:78-97`（不设置 mask_options）、`pipeline_runner.cpp:354-355`（occluder/region_masker 恒 nullptr）、`face_masker_factory.cpp`（工厂无调用点）、`task_config.ixx:161`（config 死配置）

**证据链**:
1. `MaskOptions.mask_types` 默认 `{Box}`，不含 Occlusion/Region；
2. FaceAnalysisProcessor 填充 `swap_input`/`enhance_input` 时从不设置 mask_options → 永远默认 `{Box}`；
3. `ProcessorContext::occluder`/`region_masker` 从未被赋值（恒 nullptr）；
4. `create_occlusion_masker`/`create_region_masker` 工厂存在但全项目无调用点；
5. `MaskCompositor::compose` 的 Occlusion/Region 分支需 mask_types 含之且 occluder 非空——两者都不满足 → **只有 Box 分支实际执行**；
6. config 的 `face_masker.types = {box, occlusion, region}` 是死配置（从未被读取到 pipeline）。

**设计意图确认**: 可配置 box/occlusion/region，默认 box，配置其他项后取 `cv::min` 交集融合（`mask_compositor.ixx:93-120` 已实现）。Occlusion 输入固定 256×256（BiSeNet 类遮挡检测），Region 输入固定 512×512（BiSeNet 人脸解析 19 类）。

**若完整接线的质量收益（明确）**: min 交集 = 只处理"既未被遮挡、又属于人脸语义区域"的像素 → 遮挡场景质量显著提升、边缘干净（避免处理头发/背景/遮挡物），与 facefusion 官方行为一致。

**若完整接线的性能代价（关键）**: 两个 mask 模型输入尺寸固定（256/512），swapper（128 crop）与 enhancer（512 crop）的裁剪是**同一张脸的不同分辨率标准裁剪（内容同构）**，均被 resize 到固定尺寸 → **两次推理输入几乎相同 = 近似重复推理**。

| 场景 | 每脸分割推理次数 | 每脸额外开销 | 结论 |
| :--- | :--- | :--- | :--- |
| 仅 swapper + 不共享 | 2 次（occ+region） | ~11-28ms | 可接受（相对 inswapper 主推理仍占大比例） |
| swapper+enhancer + 不共享 | 4 次 | ~22-56ms | **大幅性能损失**（约占总帧耗时 30-50%）❌ |
| swapper+enhancer + **共享** | 2 次（512 尺度算一次缩放复用） | ~11-28ms | **可接受**（与 GFPGAN 主推理同级）✅ |

**结论**: 质量收益明确，性能收益 > 代价，但**硬约束是共享 mask 结果**（FrameData 缓存 512 尺度 mask，swapper 缩放复用），不能各自推理。默认 Box 保底，Occlusion/Region 作为可配置选项。

**接线改动面**: PipelineRunner 按 types 惰性创建 occluder/region_masker（模型路径走 model_repo）→ ProcessorContext 赋值传递 → FaceAnalysisProcessor 从 config 读取 masker types/regions 填充 mask_options → （可选）FrameData 增加 mask 缓存槽共享。

### P1-3 🟠 CPU 预处理/后处理链冗余拷贝

**位置**: `inswapper.cpp:124-141, 149-186`；ArcFace/GFPGAN 同模式

**机理**: 每脸每帧 `cv::split` → `convertTo` → 3×`memcpy`；输出 3×`.clone()` → `merge` → `convertTo`。全部 batch=1、每次推理 CPU↔GPU 同步一次。

### P1-4 🟠 TaskManager 持锁等待超时，阻塞所有管理操作

**位置**: `task_manager.cpp:189-207`

**机理**: worker_loop 在持有 `mutex` 状态下 `wait_for`，期间 `submit/cancel/set_priority/进度更新` 全部阻塞；且 `listener` 同步调用（WebSocket 推送慢会拖慢执行线程）。

### P1-5 🟠 多 worker 并发抢 GPU，无显式串行化

**位置**: `pipeline_impl.ixx:96-117` + `inference_session.cpp:392-397`（`run()` 无锁）

**机理**:
- 默认 `worker_thread_count = 硬件核/2`，多个 worker 并发调用**同一 Ort::Session::Run**（ORT 声称线程安全，但 GPU 侧隐式串行 + 并发拷贝开销）；
- `PipelineConfig::max_concurrent_gpu_tasks = 2` 字段**已定义但从未使用**——本应是 GPU 并发闸门。

---

## 6. P2 级问题（次要/记录）

| # | 问题 | 位置 |
| :--- | :--- | :--- |
| P2-1 | 侧脸帧最多 **4 次 detector 推理**（0/90/180/270° 循环） | `face_analyser.cpp:108-130` |
| P2-2 | **视频间串行**：多视频任务逐个处理，不并行 | `pipeline_runner.cpp:237-243` |
| P2-2a | **SessionPool `max_entries=3` 单任务已超限**：换脸+增强+分割组合需 6 个 session > 3 → LRU 驱逐/重建（并行会加剧） | `session_pool.ixx:23` |

> **P2-2 多视频并行资源开销专项评估（2026-08-28）**:
>
> **前提**: 所有模型 session 经 `InferenceSessionRegistry` 全局共享（key = model_path+options）→ **模型权重显存只占一份**，不随并行数翻倍。
>
> **显存 (VRAM)**: 增量来自每任务 GPU 中间张量 + TRT 执行上下文 + 帧上传缓冲（~100-300MB/任务）。**2 并行 ≈ +10-20% 可控**；但 `max_entries=3` 单任务已超限（6 session > 3），并行加剧 LRU 驱逐 → 显存抖动 + TRT 引擎重载（秒级）。>4 并行显存压力显著。
>
> **内存 (RAM)**: 每并行视频需独立 VideoReader（ffmpeg 解码缓冲）+ VideoWriter + Pipeline 队列（Strict 模式 4 帧×~6MB）+ 服务对象（InSwapper initializer 1MB 等）≈ **100-300MB/视频，线性增长**。2-4 并行可接受。
>
> **收益面**: GPU 推理串行 → 并行收益仅在 **CPU-bound 场景**（解码/warp/color match/paste_back/编码密集时与 GPU 推理重叠）；GPU 满载时无收益甚至有害。
>
> **结论**: 开销本身可控（建议并行上限 2），但收益面窄（仅 CPU-bound）+ 前置依赖 SessionPool 扩容（max_entries 调至 8-12）+ 并行前 free VRAM 检查 → **ROI 中等偏低，不建议优先投资**。
| P2-3 | 全部模型 **batch=1**，同帧多人脸/帧间无 batch 累积 | 各 model impl |
| P2-4 | SessionPool `max_entries=3`：换脸+增强+检测+关键点+识别同时用 → LRU 驱逐重建 | `session_pool.ixx:22-26` |
| P2-5 | FaceEnhancerAdapter `frame.image.clone()` 整帧拷贝 | `pipeline_adapters.ixx:242` |
| P2-6 | FaceStore 空 faces 不缓存（`face_store.cpp:72`）→ 无脸帧永远重检测 | `face_analyser.cpp:135` |
| P2-7 | Strict 模式 checkpoint **每帧保存**（无 `%100` 优化，普通路径有） | `runner_video.cpp:790-798` vs `:277` |
| P2-8 | **宣传与实现落差**：README 宣称 "TensorRT + maximum throughput"，实际是 ONNX Runtime（EP 可选 TRT），且"多线程"在 GPU 场景收益有限 | `inference_session.cpp` |

---

## 7. 推理会话与推理池设计评估（InferenceSession + SessionPool）

> 专项评估（2026-08-28）: 覆盖 `src/foundation/ai/` 下 `inference_session.{ixx,cpp}`、`session_pool.{ixx,cpp}`、`inference_session_registry.{ixx,cpp}`。

### 7.1 当前实际运行状态（vs 设计意图）

| 机制 | 设计意图 | 实际状态 |
| :--- | :--- | :--- |
| SessionPool 容量/TTL 配置 | 由 `app.yaml` / `configure()` 驱动 | ❌ **`configure()` 无调用点** → 永远默认 `max_entries=3, TTL=60s`；`app.yaml` 的 `engine_cache.max_entries` 是死配置 |
| TTL 过期清理 | `cleanup_expired()` 定期释放空闲 session | ❌ **无调度者**（无后台线程/定时器调用）→ **TTL 形同虚设**，session 只靠 LRU 超限驱逐 |
| 模型预加载 | `preload_session()` 预热 | ❌ **无调用点**，机制未使用 |
| 进程内 session 复用 | LRU 缓存按 `model_path+options` 命中 | ✅ 生效（跨任务复用） |
| TRT 引擎文件缓存 | `./.cache/tensorrt` | ✅ 生效（默认路径，相对 cwd） |

### 7.2 InferenceSession 评估

**设计亮点** ✅
1. PIMPL 封装 ORT，接口干净，领域层无 ORT 侵入；
2. EP 自动检测（TRT > CUDA > CPU）+ `FACEFUSION_PROVIDER=cpu` 强制降级（CI 友好）；
3. TRT 引擎缓存 + 嵌入引擎（epContext）双模式；
4. 静态 `Ort::Env` 泄漏防析构顺序问题（经验性正确做法）；
5. `load_model` 幂等：同模型同 options 跳过重载（`inference_session.cpp:296-300`）。

**问题** ⚠️

| # | 问题 | 位置 | 严重度 |
| :--- | :--- | :--- | :--- |
| S-1 | `run()` 无锁 + 共享 `m_run_options`：依赖 ORT "Session::Run 线程安全"承诺；若未来设置 RunOptions 终止标志即数据竞争 | `inference_session.cpp:392-397` | P1（多 worker 并发 run 已踩边界） |
| S-2 | `load_model`（加锁）与 `run`（无锁）无统一并发防护；并发时 `reset_internal()` 释放 `m_ort_session` 造成 UAF——当前靠"任务内先 load 后 run"顺序侥幸安全 | `inference_session.cpp:129-147, 392-397` | P2 |
| S-3 | `is_model_loaded()` 非原子无锁读（跨线程数据竞争，实际场景安全） | `inference_session.cpp:384-386` | P2 |
| S-4 | EP 失败处理不一致：CUDA append 失败 `throw`，TRT append 失败仅 `warn` 静默降级 | `inference_session.cpp:163-169, 280-283` | P2 |
| S-5 | TRT 缓存路径相对/绝对混用（ORT 限制所致，但脆弱） | `inference_session.cpp:204-274` | P2 |

### 7.3 SessionPool 评估

**设计亮点** ✅
1. 双链表 + unordered_map 标准 LRU（O(1) 访问/驱逐）；
2. `get_or_create(key, factory)` 接口干净，池不感知模型细节；
3. 统计（hits/misses/evictions）内建，可观测性好；
4. key 生成含 EP/设备/TRT 参数（`registry.cpp:38-54`），不同配置不串用。

**问题** 🔴（比 InferenceSession 严重）

| # | 问题 | 机理 | 严重度 |
| :--- | :--- | :--- | :--- |
| P-1 | `get_or_create` 持池锁调用 factory | factory 内 `load_model` 构建 TRT 引擎可耗时数秒，期间**全局阻塞**所有 session 的 get/evict/cleanup（单 mutex）。多任务/多线程并发时是硬瓶颈 | **P0**（并行场景放大器） |
| P-2 | `max_entries=3` 单任务已超限 | 换脸+增强+分割 = 6 session > 3 → LRU 驱逐/重建常态，显存抖动 + TRT 重载秒级延迟 ✅ **已修复（`ca2afeb`）**：默认 3→10 | P1 |
| P-3 | TTL 无调度者 | `cleanup_expired()` 无调用点 → 空闲 session 永不释放 | P1 |
| P-4 | key 不含模型文件指纹 | 模型文件热更新（同路径覆盖）后 key 不变 → **命中旧 session，新权重不生效**，需重启进程 | P1（运维坑） |
| P-5 | LRU 驱逐不感知引用计数 | 被任务持有的 session 被驱逐后，旧任务继续用（shared_ptr 保护，安全），但新任务重建 → 无谓显存/时间开销 | P2 |

### 7.4 Registry 评估

- 全局单例 + key 生成合理 ✅；
- `preload_session`（先 evict 再 get_or_create）设计正确但未使用 ❌；
- `m_cache_path` 仅经 `configure()` 注入——configure 无调用 → **引擎缓存路径退化为默认 `./.cache/tensorrt`（相对 cwd）**，这也解释了运行目录必须固定（AGENTS.md 的 Cwd 规范）⚠️。

### 7.5 总体结论

**设计意图成熟**（双层缓存、LRU+TTL、key 隔离、预加载），但**实现完整度不一致**：
1. **当前架构（单任务串行）下影响有限**：session 访问实际串行，P-1 持锁问题不暴露；S-1 多 worker 并发 run 已在踩边界；
2. **一旦多任务并行（P2-2）**：P-1（池锁阻塞）、P-2（容量超限）、P-3（TTL 失效）三个问题叠加，session 层将成为新的瓶颈和显存抖动源；
3. **最值得立即修的三件事**：
   - 🥇 **P-2**: `max_entries` 3→8-12（一行配置，立即消除单任务驱逐）；
   - 🥇 **P-4**: key 加入模型文件指纹（size+mtime 或 hash），解决热更新失效；
   - 🥈 **P-3**: 接入 TTL 调度（线程或并入现有周期任务）；
   - （P-1 池锁问题 → factory 移出池锁或 per-key 锁，留待并行时处理）。

---

## 8. 合理性结论

**设计合理之处**（不建议动）:
- ✅ 任务级串行是 GPU-bound 场景的**正确保守选择**（多任务并行 GPU 无收益）
- ✅ 帧级流水线 + 保序输出 + 生产者/消费者解耦
- ✅ Session 三层缓存、模型懒加载、持久化/超时/恢复语义
- ✅ 模板对齐、Mask 组合、颜色匹配——完整复刻 facefusion 算法语义

**核心矛盾**: Pipeline 的"多线程并行"设计理念来自 CPU 场景；在 GPU 串行推理下，真正的瓶颈是 **CPU 预处理链（warp/split/memcpy/paste_back）+ 每帧重复计算 + GPU 空闲等待**。优化应聚焦"减少每帧 CPU 工作"而非"增加并行度"。

---

## 9. 优化建议（按 ROI 排序）

| 优先级 | 优化项 | 对应问题 | 预期收益 | 工作量 |
| :--- | :--- | :--- | :--- | :--- |
| 🥇 | 视频路径禁用 FaceStore / 哈希降采样 | P0-1 | 省 6-12ms/帧 | 小 |
| 🥇 | InSwapper embedding 变换任务级缓存 | P0-2 | 省 O(512²)/脸/帧 | 极小 |
| 🥇 | swapper/domain 服务跨视频复用（提升到 Impl 级） | P0-3 | 省 ONNX 重解析/视频 | 中 |
| 🥈 | Strict 模式 queue 上限放宽（如 8-16）或仅强制回收策略 | P1-1 | 提升流水线吞吐 | 小 |
| 🥈 | Mask 结果在 FrameData 共享（一次推理两处用） | P1-2 | 换脸+增强省 50% mask 开销 | 中 |
| 🥈 | 实现 `max_concurrent_gpu_tasks` 信号量闸门（或默认 worker=1） | P1-5 | 消除 GPU 竞争 | 中 |
| 🥉 | 多视频并行（TaskManager 引入 worker pool） | P2-2 | 多视频吞吐 | 大（需重构调度） |
| 🥉 | 同帧多人脸 batch 推理 | P2-3 | GPU 利用率 | 大 |

---

## 10. 待讨论问题（Open Questions）

1. **P0-3 的取舍**: swapper/domain 服务跨视频复用会引入状态共享，与"每任务隔离"的容错目标冲突——是否接受任务级复用？
2. **P1-5 的并发模型**: 默认 worker=1（GPU 串行）vs 实现 `max_concurrent_gpu_tasks` 信号量（CPU/GPU 重叠）vs 完全移除多线程——哪种符合项目定位？
3. **P2-2 的调度重构**: TaskManager 引入 worker pool 是否纳入规划（涉及持久化恢复语义与优先级语义变更）？
4. **P2-8 的宣传口径**: README 的 "TensorRT + maximum throughput" 表述是否需要与实际（ONNX Runtime + EP 可选）对齐？
5. **性能验证手段**: 是否引入基准测试（如每阶段 GPU/CPU 耗时拆分）以量化优化收益？