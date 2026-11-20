# 产品规划路线图 (Roadmap)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-PROJECT-ROADMAP-2026
> - **当前版本 (Version)**: V1.3.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-11-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.3.0** | 2026-11-20 | AI Agent | 王辉 | 追加「推理 Provider 配置接线」里程碑：闭合 BL-2/BL-3，`inference.default_providers` 生效为优先级序列。 |
| **V1.2.0** | 2026-09-28 | AI Agent | 王辉 | 追加「流程减法治理」里程碑：依据技能覆盖度分析剥离重复方法论、处置模板并确立跨技能冲突裁决。 |
| **V1.1.0** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P0–P4 闭环后更新路线图：知识库治理移入已闭环，登记六象限迁移与 CI 门禁成果。 |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 建立路线图，依据已闭环计划文档、评估报告与当前分支状态归纳产品演进阶段。 |

---

## 当前版本线

| 项 | 值 |
| :--- | :--- |
| 版本 | `0.34.1`（见 [project.yaml](../../project.yaml)） |
| 分支模型 | Git Flow：`dev` 集成分支 + `feature/*` / `fix/*` 临时分支 |
| 发布通道 | `release/*` → `master` 打 Tag 触发 [release.yml](../../.github/workflows/release.yml) |

## 演进阶段

### 已闭环（历史里程碑）

| 主题 | 交付内容 | 证据 |
| :--- | :--- | :--- |
| 性能优化 P0 | 会话池容量、FaceStore 缓存、InSwapper 初始化器解析复用、领域服务复用 | [perf-p0/IMPLEMENTATION_PLAN.md](plans/perf-p0/IMPLEMENTATION_PLAN.md) |
| 性能优化 P1（批次 A/B） | GPU 并发闸门、会话 Key 指纹、TTL 惰性清理、池锁重构、mask 链路接线、checkpoint 节流 | [perf-p1a](plans/perf-p1a/IMPLEMENTATION_PLAN.md)、[perf-p1b](plans/perf-p1b/IMPLEMENTATION_PLAN.md) |
| 性能优化 P2（次要项） | 全帧 clone 跳过、addWeighted in-place UB 修复、README 口径校准 | [perf-p2minor/IMPLEMENTATION_PLAN.md](plans/perf-p2minor/IMPLEMENTATION_PLAN.md) |
| Web 健壮性 | 配置序列化/接线、任务持久化、执行超时、WebSocket 稳定性 | [web-robustness/IMPLEMENTATION_PLAN.md](plans/web-robustness/IMPLEMENTATION_PLAN.md) |

### 已闭环（近期）

| 主题 | 交付内容 | 证据 |
| :--- | :--- | :--- |
| 流程减法治理 | 方法层让位技能、契约层留在仓库：workflow 306→166 行（新增技能路由与跨技能冲突裁决）、模板 704→262 行（task 按实际结构重写、evaluation 归档）、C++_quality_standard §7 归档 | [backlog.md](./backlog.md) |
| 推理 Provider 配置接线 | `inference.default_providers` 从"解析后丢弃"变为真正决定会话 EP（优先级 + 可用性过滤 + 闭包展开），PipelineRunner 与 Web 人脸检测统一走 `config.inference_options`；FAQ/配置指南口径对齐 | [provider-wiring 计划](./plans/provider-wiring/IMPLEMENTATION_PLAN.md) |
| 知识库治理（Diátaxis 标准化） | P0 自愈（健康度 37.2 → 100.0，断链 41 → 0、控制头 68/68）→ P1 立骨架（GOVERNANCE/llms.txt/象限）→ P2 六象限迁移（67 篇）→ P3 拆分 design.md（1052 行 → 总纲 + 5 子册）→ P4 规范同步与 CI 门禁 | [docs-governance 计划](./plans/docs-governance/IMPLEMENTATION_PLAN.md) |


### 待评估 / 待裁决

| 主题 | 触发来源 | 门禁条件 |
| :--- | :--- | :--- |
| 非 NVIDIA GPU 加速（Vulkan 路径） | 远端 issue #1「add support vulkan」 | ORT WebGPU plugin EP 的 PoC 三个判定点（节点覆盖 / 并发稳定性 / 数值一致性）通过；详见 [vulkan-backend-feasibility-review.md](../explanation/zh/analysis/vulkan-backend-feasibility-review.md) |

## 说明

- 本路线图只登记**已发生事实**与**已立项事项**，不做无依据的时间承诺；
- 待办与技术债明细见 [backlog.md](backlog.md)；逐版本变更见 [changelog.md](changelog.md)。
