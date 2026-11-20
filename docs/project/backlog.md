# 待办与技术债汇总 (Backlog)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-PROJECT-BACKLOG-2026
> - **当前版本 (Version)**: V1.4.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-11-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.4.0** | 2026-11-20 | AI Agent | 王辉 | BL-2 / BL-3 闭环：`inference.default_providers` 经 `config.inference_options` 接线为优先级序列并覆盖 PipelineRunner 与 Web 人脸检测两条路径；FAQ 与配置指南口径对齐。新登记 BL-9（`engine_cache.max_entries` / `idle_timeout_seconds` 解析未接线）。 |
| **V1.3.0** | 2026-09-28 | AI Agent | 王辉 | 台账清理与 BL-8 闭环：补齐此前遗漏登记的已闭环条目（BL-4 拆分、BL-7 索引控制头、流程减法治理），BL-8 因技能包补齐 brainstorming / writing-plans 而闭环；修正 BL-3 来源路径，消除条目间多余空行。 |
| **V1.2.0** | 2026-09-28 | AI Agent | 王辉 | 流程减法治理：登记 BL-8（技能侧悬空引用），闭环流程文档与模板治理项，修正编号重复。 |
| **V1.1.0** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P3 闭环 BL-4（design.md 拆分）；BL-3 来源路径随 Diátaxis 迁移更新。 |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 建立待办汇总，收录 Vulkan 评估风险项、配置未接线项与文档治理遗留项。 |

---

## 未闭环事项

| ID | 事项 | 来源 | 优先级 | 状态 |
| :--- | :--- | :--- | :--- | :--- |
| **BL-1** | 非 NVIDIA GPU 加速：接入 ORT WebGPU plugin EP，需先通过 PoC 三判定点 | [vulkan-backend-feasibility-review.md](../explanation/zh/analysis/vulkan-backend-feasibility-review.md) R-1~R-8、issue #1 | P0 | 待 PoC |
| **BL-5** | `FACEFUSION_ENABLE_GPU` 为空转开关：`build.py` 传入 CMake 但全仓库无消费点 | Vulkan 评估 §3.2 | P2 | 未开始 |
| **BL-6** | [compiler_selection.md](../explanation/zh/decisions/compiler_selection.md) 按 MADR 3.0 规范重构（编号 `0001-xxx.md` 并补决策矩阵） | 治理计划 §5.4 范围外事项 | P2 | 未开始 |
| **BL-9** | `inference.engine_cache.max_entries` / `idle_timeout_seconds` 解析后无消费点：`SessionPool` 使用自身默认容量 10，未接收配置 | provider-wiring 计划 §1.3（`app_config.ixx:29-30`、`session_pool.ixx:23`） | P2 | 未开始 |

## 已闭环事项

| ID | 事项 | 闭环证据 |
| :--- | :--- | :--- |
| — | 性能优化 P0/P1/P2 全部审查项 | 提交 `920a1fb`：`docs(eval): mark P2 minor items fixed; all review items closed` |
| — | 知识库断链 41 处、控制头缺失 16 篇、孤儿文档 14 篇、修订历史超标 5 篇 | 提交 `decc53f`：治理 P0 自愈，健康度 100.0/100 |
| **BL-3** | 用户 FAQ 与实现不一致 | [faq.md](../how-to/zh/faq.md) V1.1.0 / [configuration_guide.md](../reference/zh/configuration_guide.md) V1.4.0（en 同步）；语义口径见 [provider-wiring 计划](./plans/provider-wiring/IMPLEMENTATION_PLAN.md) D1–D4 |
| **BL-2** | `inference.default_providers` 配置未接线 | `fix/task-inference-provider-wiring`：新增 `config.inference_options`（`src/app/config/inference_options.ixx`，10 条单测）；接线 `pipeline_runner.cpp` 与 `app_cli.cpp`；全量单测 329/329 |
| **BL-4** | `design.md` 1052 行超出单文档 800 行红线 | 治理 P3：拆分为总纲（113 行）+ 5 份子册（110–375 行），零断链 |
| **BL-7** | `docs/index.md` 控制头缺少列表前缀 | 治理 P4：控制头统一为 `> - **键**` 格式，index 升版 V2.0.0 |
| — | 流程文档与技能库大量重复、模板采用率分化（task 0/17、evaluation 0/3） | 流程减法治理：workflow 306→166 行、模板 704→262 行、§7 评分标准归档、新增跨技能冲突裁决 |
| **BL-8** | `goal-loop` 声明的 `writing-plans` / `brainstorming` 技能缺失，外部联动无法激活 | 技能包已补齐：`brainstorming`（285 行）、`writing-plans`（204 行），技能总数 88→103；已在 [workflow.md](../reference/zh/rules/workflow.md) §1 技能路由表登记配套位置 |

## 说明

- 优先级定义：**P0** 影响对外承诺或阻塞其他工作；**P1** 影响正确性或用户可见行为；**P2** 工程质量改进；**P3** 打磨项；
- 新事项登记时必须写明**来源**（文档链接或提交号），无来源的设想请先进入 [proposals/](../proposals/README.md) 孵化。
