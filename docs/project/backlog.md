# 待办与技术债汇总 (Backlog)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-PROJECT-BACKLOG-2026
> - **当前版本 (Version)**: V1.8.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-11-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.8.0** | 2026-11-20 | AI Agent | 王辉 | 新增 BL-10：TaskManager 在持有状态锁时执行快照落盘/日志，磁盘抖动会阻塞 `get()`/`submit()`（lost-wakeup 修复过程中发现）。 |
| **V1.7.0** | 2026-11-20 | AI Agent | 王辉 | BL-6 闭环：ADR 重编号 `0001-compiler-selection.md` 并补齐 MADR 3.0 结构（驱动因素、后果、加权决策矩阵）。 |
| **V1.6.0** | 2026-11-20 | AI Agent | 王辉 | BL-5 闭环：`--enable-gpu` 由空转开关改为显式 no-op + 运行告警，`build.py` 不再传无消费点的 CMake 参数。 |
| **V1.5.0** | 2026-11-20 | AI Agent | 王辉 | BL-9 闭环：`engine_cache.max_entries` / `idle_timeout_seconds` 接入 `SessionPool`（默认容量 3→10）；启动日志新增 `Session Pool:` 行，`--validate` 冒烟确认 4/30 配置生效。 |
| **V1.4.0** | 2026-11-20 | AI Agent | 王辉 | BL-2 / BL-3 闭环：`inference.default_providers` 经 `config.inference_options` 接线为优先级序列并覆盖 PipelineRunner 与 Web 人脸检测两条路径；FAQ 与配置指南口径对齐。新登记 BL-9（`engine_cache.max_entries` / `idle_timeout_seconds` 解析未接线）。 |

---

## 未闭环事项

| ID | 事项 | 来源 | 优先级 | 状态 |
| :--- | :--- | :--- | :--- | :--- |
| **BL-1** | 非 NVIDIA GPU 加速：接入 ORT WebGPU plugin EP，需先通过 PoC 三判定点 | [vulkan-backend-feasibility-review.md](../explanation/zh/analysis/vulkan-backend-feasibility-review.md) R-1~R-8、issue #1 | P0 | 待 PoC |
| **BL-10** | TaskManager 在持有状态锁时执行快照落盘与日志（`save_snapshot` 于 `submit`/pick/完成/`cancel`/`set_priority`）：磁盘抖动时 `get()`/`submit()` 被一并阻塞 | lost-wakeup 诊断（`fix/task-taskmanager-lost-wakeup`）| P2 | 未开始 |

## 已闭环事项

| ID | 事项 | 闭环证据 |
| :--- | :--- | :--- |
| — | 性能优化 P0/P1/P2 全部审查项 | 提交 `920a1fb`：`docs(eval): mark P2 minor items fixed; all review items closed` |
| — | 知识库断链 41 处、控制头缺失 16 篇、孤儿文档 14 篇、修订历史超标 5 篇 | 提交 `decc53f`：治理 P0 自愈，健康度 100.0/100 |
| **BL-3** | 用户 FAQ 与实现不一致 | [faq.md](../how-to/zh/faq.md) V1.1.0 / [configuration_guide.md](../reference/zh/configuration_guide.md) V1.4.0（en 同步）；语义口径见 [provider-wiring 计划](./plans/provider-wiring/IMPLEMENTATION_PLAN.md) D1–D4 |
| **BL-6** | `compiler_selection.md` 未按 MADR 3.0 组织 | `fix/task-madr-adr`：重编号为 [0001-compiler-selection.md](../explanation/zh/decisions/0001-compiler-selection.md) V2.0.0，补齐决策驱动因素/后果/加权决策矩阵（4.65 vs 3.50 vs 1.70）|
| **BL-5** | `FACEFUSION_ENABLE_GPU` 为空转开关 | `fix/task-enable-gpu-flag`：GPU 支持本就无条件编译，`--enable-gpu` 改为**显式 no-op**（帮助文本 + 运行告警），不再向 CMake 传死参数；`tests/scripts/test_build_py.py` 改为回归守卫；[build.md](../how-to/zh/build.md) V1.3.0 |
| **BL-9** | `engine_cache.max_entries` / `idle_timeout_seconds` 解析后无消费点 | `fix/task-engine-cache-config`：新增 `config::make_pool_config`（3 条单测）+ `App::configure_inference_registry` 启动接线；默认容量 3→10；运行时冒烟 `capacity=4, ttl=30s` |
| **BL-2** | `inference.default_providers` 配置未接线 | `fix/task-inference-provider-wiring`：新增 `config.inference_options`（`src/app/config/inference_options.ixx`，10 条单测）；接线 `pipeline_runner.cpp` 与 `app_cli.cpp`；全量单测 329/329 |
| **BL-4** | `design.md` 1052 行超出单文档 800 行红线 | 治理 P3：拆分为总纲（113 行）+ 5 份子册（110–375 行），零断链 |
| **BL-7** | `docs/index.md` 控制头缺少列表前缀 | 治理 P4：控制头统一为 `> - **键**` 格式，index 升版 V2.0.0 |
| — | 流程文档与技能库大量重复、模板采用率分化（task 0/17、evaluation 0/3） | 流程减法治理：workflow 306→166 行、模板 704→262 行、§7 评分标准归档、新增跨技能冲突裁决 |
| **BL-8** | `goal-loop` 声明的 `writing-plans` / `brainstorming` 技能缺失，外部联动无法激活 | 技能包已补齐：`brainstorming`（285 行）、`writing-plans`（204 行），技能总数 88→103；已在 [workflow.md](../reference/zh/rules/workflow.md) §1 技能路由表登记配套位置 |

## 说明

- 优先级定义：**P0** 影响对外承诺或阻塞其他工作；**P1** 影响正确性或用户可见行为；**P2** 工程质量改进；**P3** 打磨项；
- 新事项登记时必须写明**来源**（文档链接或提交号），无来源的设想请先进入 [proposals/](../proposals/README.md) 孵化。
