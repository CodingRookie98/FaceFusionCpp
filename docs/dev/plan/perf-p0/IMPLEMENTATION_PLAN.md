# 性能优化 P0 批次 实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-PERF-P0-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-28 | AI Agent | 王辉 | 依据评估报告（[architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)）P0 优先级创建：消除视频路径 FaceStore 整帧哈希、InSwapper embedding 变换任务级缓存、domain 服务跨视频复用、SessionPool 容量修正。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../../zh/process/workflow.md)
> *   评估报告: [architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块接口（domain.face.analyser / domain.face.store / domain.face.swapper / services.pipeline.runner / foundation.ai.session_pool）。
*   [x] 我已确认新增接口不与现有冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程 (🔴 Red → 🟢 Green → 🔵 Refactor)。

**已检查的上下文:**
*   文件 1: `src/domain/face/analyser/face_analyser.cpp` —— `get_many_faces()` 每帧执行 `is_contains` + `insert_faces`（各一次整帧 FNV1a 哈希，L66/L170）；4 角度检测循环（L108-130）。
*   文件 2: `src/domain/face/face_store.cpp` —— `get_key()` 全帧逐字节哈希（L161-184）；`insert_faces` 空 faces 不缓存（L72）。
*   文件 3: `src/domain/face/swapper/impl/inswapper.cpp` —— `prepare_input()` 中 embedding 变换 O(n²) 每次重算（L111-118）；`load_model()` 每次 `init()` 重新解析 ONNX protobuf + FP16→FP32 转换（L29-84）。
*   文件 4: `src/services/pipeline/pipeline_runner.cpp` —— `AddProcessorsToPipeline()` 内 `domain_ctx` 为局部变量（L352），每视频/每批次重新创建 swapper/enhancer 并 `load_model`（L380/L410/L439）。
*   文件 5: `src/foundation/ai/session_pool.ixx` —— `PoolConfig.max_entries` 默认 3（L23），单任务 6 session 超限。
*   文件 6: `tests/unit/app/` 与 `tests/unit/domain/` —— 现有测试基建。

## 1. 计划概述

### 1.1 目标与范围

*   **核心目标**（对齐评估报告 §4 P0 三项 + §7.5 会话容量修正）:
    1. **P0-1 视频路径消除 FaceStore 整帧哈希**: 视频帧每帧 2 次全帧 FNV1a 哈希（~6-12ms/帧）纯浪费；FaceAnalyser 增加缓存开关，视频处理路径禁用（仅图像/跨任务复用保留）。
    2. **P0-2 InSwapper embedding 变换任务级缓存**: `prepare_input()` 中 512×512 矩阵向量乘结果在任务内恒定，缓存后每脸/每帧省 O(512²)。
    3. **P0-3 domain 服务跨视频复用**: `AddProcessorsToPipeline` 每视频重建 swapper/enhancer 并重新解析 ONNX 初始化器；按（步骤类型 + 模型名）缓存实例到 Impl 级。
    4. **Session-2 SessionPool 容量修正**: `max_entries` 默认 3→10，消除单任务（6 session）LRU 驱逐/重建。

*   **涉及模块**: `domain.face.analyser`、`domain.face.store`、`domain.face.swapper`、`services.pipeline.runner`、`foundation.ai.session_pool`、测试。
*   **范围外**（后续批次，见评估报告 §8/§9）: P1-1 Strict 模式 queue 上限、P1-2 mask 接线+共享、P1-5 GPU 并发闸门、P2-2 多视频并行、Session P-1/P-3/P-4（池锁、TTL 调度、key 指纹）。

### 1.2 成功标准

1. 视频处理路径不再执行整帧哈希（可观测：FaceAnalyser 缓存关闭后 `FaceStore` 无视频帧写入）。
2. 同一任务内多次 `swap_face()` 只计算一次 embedding 变换（单测验证）。
3. 多视频任务中 swapper/enhancer 对象与 ONNX 初始化器只创建/解析一次（单测或计数器验证）。
4. `SessionPool` 默认容量 10，单任务 6 session 无驱逐（单测验证）。
5. 单元测试全绿、集成测试无回归。

## 2. 任务分解

| 任务 | 名称 | 说明 | 依赖 |
| :--- | :--- | :--- | :--- |
| T1 | 视频路径禁用 FaceStore 整帧哈希 | FaceAnalyser 增加缓存开关（Options/构造参数），`get_many_faces` 按开关跳过 store 访问 | - |
| T2 | InSwapper embedding 变换任务级缓存 | `InSwapper` 缓存 embedding→初始变换结果，source_embedding 未变时复用 | - |
| T3 | domain 服务跨视频复用 | `PipelineRunner::Impl` 按（步骤+模型）缓存 swapper/enhancer/restorer 实例 | T1 完成后（涉及 runner） |
| T4 | SessionPool 容量修正 | `PoolConfig.max_entries` 默认 3→10 | - |

> 各任务详细设计、TDD 用例与验收标准见 `task/C++_task_{name}.md`。

## 3. 实施阶段

### 阶段一: 任务文档生成
- [ ] 生成 4 个子任务文档（task/C++_task_*.md）

### 阶段二: 分支 + TDD 实现
- [ ] 创建分支 `feature/plan-perf-p0`
- [ ] T1 → T4 依次按 🔴 Red → 🟢 Green → 🔵 Refactor 实现
- [ ] 每任务提交 + 更新任务文档状态

### 阶段三: 集成验证
- [ ] `python build.py --action test --test-label integration` 全绿

### 阶段四: 完成验收与合并
- [ ] E2E 测试（如适用）
- [ ] 合并回 `dev`，删除分支

### 阶段五: 文档归档
- [ ] 更新评估报告（P0 条目标记已修复）、配置文档（如涉及）