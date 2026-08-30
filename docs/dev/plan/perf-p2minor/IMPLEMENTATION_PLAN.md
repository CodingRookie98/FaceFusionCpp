# 性能优化 P2 小项 实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-PERF-P2MINOR-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-29

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-29 | AI Agent | 王辉 | 验收完成：单元 319/319 全绿。T1（`b19f451`）除 P2-5 clone 优化外，顺带发现并修复 **addWeighted in-place UB**（dst==src2 导致 blend<100 输出无变化，真实缺陷）；T2（`baf2b70`）README/README_CN 口径校准。 |
| **V1.0.0** | 2026-08-28 | AI Agent | 王辉 | P2 小项批次：P2-5（FaceEnhancerAdapter blend=100 跳过整帧 clone）+ P2-8（README 宣传口径校准）。P2-1（4 角度检测，标准鲁棒性做法）与 P2-3（batch，模型 batch=1 硬约束）经讨论确认**不做**。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   工作流程: [workflow.md](../../zh/process/workflow.md)
> *   评估报告: [architecture-performance-review.md](../../zh/evaluation/architecture-performance-review.md)

## 0. 计划前验证 (AI Agent 自检)

*   [x] 我已阅读 [AGENTS.md](../../../AGENTS.md) 中的 C++20 开发规范。
*   [x] 我已阅读相关现有模块接口（domain.pipeline:adapters FaceEnhancerAdapter / README.md）。
*   [x] 我已确认新增接口不与现有冲突。
*   [x] **TDD 承诺**: 本计划所有实施阶段将严格遵循 TDD 流程。

**已检查的上下文:**
*   文件 1: `src/domain/pipeline/pipeline_adapters.ixx:242` —— `cv::Mat working_frame = frame.image.clone();`（enhance 路径无条件 clone；blend>=100 时不需要原始帧）。
*   文件 2: `README.md` / `README_CN.md` —— 宣称 "TensorRT + maximum throughput + multi-threading"；实际为 ONNX Runtime（EP 可选 TRT/CUDA/CPU）+ 帧级并行 + GPU 串行闸门。

## 1. 计划概述

### 1.1 目标与范围

1. **P2-5**: `FaceEnhancerAdapter` 在 `face_blend >= 100`（全量替换）时跳过 `frame.image.clone()`（省整帧拷贝 ~3-5ms/帧 @1080p）。
2. **P2-8**: README / README_CN 宣传口径校准——"TensorRT/CUDA 加速（ONNX Runtime Execution Providers）" + "帧级并行流水线 + GPU 推理串行化闸门"。

*   **涉及模块**: `domain.pipeline:adapters`、`README.md`、`README_CN.md`。
*   **范围外**: P2-1（4 角度检测，标准鲁棒性做法）、P2-2（多视频并行，待用户拍板）、P2-3（batch，模型 batch=1 硬约束）。

### 1.2 成功标准

1. `face_blend >= 100` 时不再 clone 原始帧（行为不变：全量替换）。
2. `face_blend < 100` 时行为不变（加权混合）。
3. README 措辞与实际（ORT EP + 帧级并行 + GPU 闸门）一致。
4. 单元测试全绿、集成无回归。

## 2. 任务分解

| 任务 | 名称 | 说明 | 依赖 |
| :--- | :--- | :--- | :--- |
| T1 | P2-5 blend=100 跳过 clone | FaceEnhancerAdapter 小优化 + 行为测试 | - |
| T2 | P2-8 README 口径校准 | README / README_CN 措辞更新（纯文档） | - |

## 3. 实施阶段

### 阶段一: 分支 + 实现
- [x] 创建分支 `feature/plan-perf-p2minor`
- [x] T1（`b19f451`，含 addWeighted UB 修复）→ T2（`baf2b70`）实现
- [x] 每任务提交

### 阶段二: 验证与合并
- [x] 单元全量 319/319 通过
- [ ] 集成全量（后台进行中）
- [ ] 合并回 `dev`，删除分支

### 阶段三: 文档归档
- [ ] 更新评估报告（P2-5/P2-8 标注 + addWeighted bug 记录）