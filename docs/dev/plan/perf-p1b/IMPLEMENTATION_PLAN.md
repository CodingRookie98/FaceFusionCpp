# 性能优化 P1 批次（第二批）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-PERF-P1B-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-28 | AI Agent | 王辉 | 依据评估报告（[architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)）剩余优先级创建（第二批）：P1-2 mask 接线+共享（方案 B）、Web 集成 flaky 修复、P-1 SessionPool 池锁重构、P2-7 Strict checkpoint 周期保存。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../../zh/process/workflow.md)
> *   评估报告: [architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块接口（domain.face.masker / domain.pipeline:adapters / foundation.ai.session_pool / services.pipeline.runner / tests integration web_ws）。
*   [x] 我已确认新增接口不与现有冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程 (🔴 Red → 🟢 Green → 🔵 Refactor)。

**已检查的上下文:**
*   文件 1: `src/domain/face/masker/` —— MaskCompositor::compose（Box/Occlusion/Region min 融合已实现）；occluder/region_masker 从未创建（评估报告 P1-2 证据链）。
*   文件 2: `src/services/pipeline/runner_types.cpp:55-57` —— ProcessorContext 已有 occluder/region_masker 字段但从未赋值。
*   文件 3: `src/app/config/task_config.ixx:161-172` —— FaceMaskerConfig（types/region 等）已解析但未接入 pipeline。
*   文件 4: `tests/integration/app/web_ws_test.cpp:245` —— WS 3s 超时（长跑负载下 flaky）。
*   文件 5: `src/foundation/ai/session_pool.cpp:94-131` —— get_or_create 持池锁调用 factory（TRT 加载秒级阻塞全局）。
*   文件 6: `src/services/pipeline/runner_video.cpp:790-798` —— Strict 路径 checkpoint 每帧保存（普通路径有 %100 优化）。

## 1. 计划概述

### 1.1 目标与范围

1. **P1-2 mask 接线 + 共享（方案 B）**: 接通 Box/Occlusion/Region mask 链路——config → FaceAnalysisProcessor 计算（512 尺度参考 crop）→ FrameData 共享 → adapter 缩放复用；默认仍 Box 保底，Occlusion/Region 可配置启用。
2. **Web 集成 flaky 修复**: WS 测试 3s 超时放宽至 10s（长跑负载下消息延迟），消除随机失败。
3. **P-1 SessionPool 池锁重构**: factory 移出池锁（double-check + per-key 在建集合），消除 TRT 加载秒级全局阻塞。
4. **P2-7 Strict checkpoint 周期保存**: Strict 路径补 `seq_id % 100 == 0` 优化（对齐普通路径）。

*   **涉及模块**: `domain.face.masker`、`domain.pipeline:adapters`、`services.pipeline.runner`（processor/runner_types）、`app.config`（masker 接线）、`foundation.ai.session_pool`、集成测试。
*   **范围外**（后续批次）: P2-1（4 角度检测，facefusion 标准鲁棒性做法）、P2-2（多视频并行，低 ROI）、P2-3（batch，大改）、P2-5（clone 为 blend 必需）、P2-8（README 文案）。

### 1.2 成功标准

1. 配置 `face_masker.types` 含 occlusion/region 时：FaceAnalysisProcessor 生成组合 mask 并经 FrameData 共享，两个 adapter 均复用（不重复推理）。
2. 默认（box only）行为不变（零额外推理）。
3. WS 集成测试连续全量跑 3 次无 flaky。
4. SessionPool 持锁时间仅限 map 操作（factory 在锁外执行）。
5. Strict checkpoint 每 100 帧保存。
6. 单元测试全绿、集成测试无回归。

## 2. 任务分解

| 任务 | 名称 | 说明 | 依赖 |
| :--- | :--- | :--- | :--- |
| T1 | mask 链路接线 + 共享 | config→分析器→FrameData→adapter 复用 | - |
| T2 | Web WS flaky 修复 | 超时 3s→10s | - |
| T3 | SessionPool 池锁重构 | factory 移出锁 | - |
| T4 | Strict checkpoint 周期保存 | %100 优化 | - |

> 各任务详细设计、TDD 用例与验收标准见 `task/C++_task_{name}.md`。

## 3. 实施阶段

### 阶段一: 任务文档生成
- [ ] 生成 4 个子任务文档（task/C++_task_*.md）

### 阶段二: 分支 + TDD 实现
- [ ] 创建分支 `feature/plan-perf-p1b`
- [ ] T1 → T4 依次按 🔴 Red → 🟢 Green → 🔵 Refactor 实现
- [ ] 每任务提交 + 更新任务文档状态

### 阶段三: 集成验证
- [ ] 单元测试全量 + 集成测试全绿（WS 连续验证）

### 阶段四: 完成验收与合并
- [ ] E2E 测试（如适用）
- [ ] 合并回 `dev`，删除分支

### 阶段五: 文档归档
- [ ] 更新评估报告（P1-2/P-1/P2-7 条目标记已修复）