# 待办与技术债汇总 (Backlog)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-PROJECT-BACKLOG-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P3 闭环 BL-4（design.md 拆分）；BL-3 来源路径随 Diátaxis 迁移更新。 |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 建立待办汇总，收录 Vulkan 评估风险项、配置未接线项与文档治理遗留项。 |

---

## 未闭环事项

| ID | 事项 | 来源 | 优先级 | 状态 |
| :--- | :--- | :--- | :--- | :--- |
| **BL-1** | 非 NVIDIA GPU 加速：接入 ORT WebGPU plugin EP，需先通过 PoC 三判定点 | [vulkan-backend-feasibility-review.md](../explanation/zh/analysis/vulkan-backend-feasibility-review.md) R-1~R-8、issue #1 | P0 | 待 PoC |
| **BL-2** | `inference.default_providers` 配置未接线：YAML 解析后从未映射为 `ExecutionProvider`，实际全走自动探测 | 同上 §3.2（`config_parser.cpp:330-332`） | P1 | 未开始 |
| **BL-3** | 用户 FAQ 与实现不一致：`docs/user/zh/faq.md` 建议优先 TensorRT，但该配置当前不生效 | 同上 §3.2 | P1 | 未开始 |
| **BL-4** | `design.md` 1052 行超出单文档 800 行红线，需拆分为「总纲 + 子册」 | 治理计划 P3 | P1 | 待 P3 |
| **BL-5** | `FACEFUSION_ENABLE_GPU` 为空转开关：`build.py` 传入 CMake 但全仓库无消费点 | Vulkan 评估 §3.2 | P2 | 未开始 |
| **BL-6** | `compiler_selection.md` 按 MADR 3.0 规范重构（编号 `0001-xxx.md` 并补决策矩阵） | 治理计划 §5.4 范围外事项 | P2 | 未开始 |

## 已闭环事项

| ID | 事项 | 闭环证据 |
| :--- | :--- | :--- |
| — | 性能优化 P0/P1/P2 全部审查项 | 提交 `920a1fb`：`docs(eval): mark P2 minor items fixed; all review items closed` |
| — | 知识库断链 41 处、控制头缺失 16 篇、孤儿文档 14 篇、修订历史超标 5 篇 | 提交 `decc53f`：治理 P0 自愈，健康度 100.0/100 |

## 说明

- 优先级定义：**P0** 影响对外承诺或阻塞其他工作；**P1** 影响正确性或用户可见行为；**P2** 工程质量改进；**P3** 打磨项；
- 新事项登记时必须写明**来源**（文档链接或提交号），无来源的设想请先进入 [proposals/](../proposals/README.md) 孵化。
