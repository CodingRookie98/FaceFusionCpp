# 工作流程 (Workflow)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PROC-WF-2026
> - **当前版本 (Version)**: V2.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V2.0.0** | 2026-09-28 | AI Agent | 王辉 | 减法治理：本文档收敛为「项目执行契约」，通用方法论（上下文管理规则、TDD 循环详述、阶段编排、分支规范、流程图）移交技能库，新增「技能路由」章节；完整保留命令族、失败处理矩阵、产物路径与文档变更清单。 |
| **V1.1.0** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P4：路径约定对齐 Diátaxis 拓扑。 |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |

---

## 1. 定位：契约在此，方法在技能

本文档**只定义本仓库特有的执行契约**（命令、产物路径、失败阈值与文档清单）。通用工程方法论由技能库提供并随工具链演进，本文不再重复描述。

| 场景 | 技能 | 提供的能力 |
| :--- | :--- | :--- |
| 长程 / 多模块 / 跨层任务（> 10 分钟） | `goal-loop` | 阶段路由与检查点持久化、P-1 用户认可门禁、3-Tries 熔断、交付前 Zero Blockers 裁决 |
| 新功能 / 缺陷修复的编码循环 | `tdd` | Red-Green-Refactor 循环、测试缝隙选择与反模式 |
| 交付 / PR 合并前 | `dual-round-review` | 双轮对抗审查：红队穿透 + 元架构师审判 |
| 变更审查 | `code-review` | Standards 与 Spec 双轴并行审查 |
| 文档变更 | `doc-governance` | Diátaxis 拓扑、控制头基线、断链与健康度门禁 |
| 缺陷诊断 | `diagnosing-bugs` | 复现优先的诊断循环 |

> **冲突裁决**：项目特有契约（本文档与 [AGENTS.md](../../../../AGENTS.md)）优先于技能中的通用建议；方法论细节以技能为准。

**流程骨架**（方法细节见上表技能）：

| 阶段 | 产出 | 契约约束 |
| :--- | :--- | :--- |
| 评估（MAY） | 评估报告 → `docs/explanation/zh/analysis/` | 按需触发，非必需 |
| 计划 | `docs/project/plans/{plan}/IMPLEMENTATION_PLAN.md` | 未获用户明确认可不得编码 |
| 子任务 | `docs/project/plans/{plan}/task/C++_task_{name}.md` | 建议使用 [C++_task_template.md](../../../templates/C++_task_template.md) |
| 编码 | 功能分支 + 原子提交 | 见 §2.1、§2.3 |
| 验证与合并 | 测试全绿 → 合并回 `dev` | 见 §2.2、§3 |
| 文档归档 | 受影响的文档同步更新 | 见 §4 |

---

## 2. 项目特有契约

### 2.1 分支模型

完整规范见 [AGENTS.md](../../../../AGENTS.md)（单一真相源，不在本文重复）。摘要：基于 `dev` 创建 `feature/plan-{name}` 或 `fix/task-{name}`，验收后合并回 `dev` 并删除；严禁直推 `master` / `dev`。

### 2.2 测试分层

按变更类型选择测试层级，完整表格见 [AGENTS.md](../../../../AGENTS.md) 的「测试分层策略」（单一真相源）。

**项目特有例外**：纯文档变更（`docs/` 下的 `.md` 文件，无 `.cpp`/`.ixx` 改动）**可跳过全部测试直接提交**，并须在提交说明中注明跳过原因。

### 2.3 交付命令

```bash
# 单元测试（每个子任务完成即执行）
python build.py --action test --test-label unit

# 集成测试（跨模块/跨层变更必跑，10–40 分钟，建议后台执行）
python build.py --action test --test-label integration

# 完整测试套件（合并前最终验证）
python build.py --action test

# E2E（发布准备与重大架构变更；工作目录须为可执行文件所在目录）
cd build/<preset>/bin && python3 ../../../tests/e2e/scripts/run_e2e.py
```

### 2.4 产物路径约定

| 产物 | 路径 |
| :--- | :--- |
| 评估报告 | `docs/explanation/zh/analysis/C++_evaluation_{title}.md` |
| 实施计划 | `docs/project/plans/{plan_name}/IMPLEMENTATION_PLAN.md` |
| 子任务文档 | `docs/project/plans/{plan_name}/task/C++_task_{task_name}.md` |
| 技术决策 (ADR) | `docs/explanation/zh/decisions/` |
| 构建流程说明 | `docs/how-to/zh/build.md` |

---

## 3. 失败处理矩阵

> 通用重试上限与熔断规则（3-Tries、升级人工）见 [AGENTS.md](../../../../AGENTS.md)；下表为**本项目特有的分类阈值**。

**实现阶段**：

| 失败场景 | 检测方式 | 重试策略 | 回退路径 | 升级条件 |
| :--- | :--- | :--- | :--- | :--- |
| 单元测试失败 | `build.py --action test` 返回非零 | 最多重试 3 次修改 | 回退到上一个通过的 commit | 3 次失败后升级为人工介入 |
| 编译失败 | `build.py --action build` 返回非零 | 最多重试 2 次修复 | 回退到上一个编译通过的状态 | 2 次失败后检查依赖/环境 |
| 重构后测试失败 | 重构后运行测试 | 回退重构，重新尝试 | 放弃本次重构，保持原代码 | 3 次失败后跳过重构步骤 |

**集成验证阶段**：

| 失败场景 | 检测方式 | 处理策略 | 回退路径 | 升级条件 |
| :--- | :--- | :--- | :--- | :--- |
| 单个测试用例失败 | 测试报告显示具体失败用例 | 定位到具体模块后修复 | 回退到最后一次集成通过的状态 | 3 次修复失败后升级 |
| 多个测试用例失败 | 测试报告显示多个失败 | 按失败优先级逐个修复 | 回退到最后一次集成通过的状态 | 失败数 > 5 时升级 |
| 环境依赖失败 | CUDA / ONNX Runtime 不可用 | 检查环境配置，修复后重试 | 不回退代码，修复环境 | 环境无法修复时升级 |
| 超时失败 | 测试执行超过预期时间 2x | 检查是否有死锁或无限循环 | 回退到最后一次集成通过的状态 | 超时 > 10min 时升级 |

---

## 4. 文档变更检查清单（合并后执行）

**开发文档**：

| 文档 | 检查条件 | 路径 |
| :--- | :--- | :--- |
| 架构设计 | 是否新增/修改了模块、分层、接口 | `explanation/zh/architecture/design.md`（总纲 + 子册） |
| 分层结构 | 是否调整了层级依赖关系 | `explanation/zh/architecture/layers.md` |
| 构建指南 | 是否新增/变更了构建依赖或配置 | `how-to/zh/setup.md`、`how-to/zh/build.md` |
| 质量标准 | 是否新增/修改了代码规范 | `reference/zh/rules/C++_quality_standard.md` |
| 工作流程 | 是否调整了开发流程 | `reference/zh/rules/workflow.md` |
| 疑难杂症 | 是否解决了新的技术问题 | `how-to/zh/troubleshooting/README.md` |

**用户文档**：

| 文档 | 检查条件 | 路径 |
| :--- | :--- | :--- |
| 快速上手 | 是否影响安装/首次使用流程 | `tutorials/zh/getting_started.md` |
| 用户指南 | 是否新增/修改了功能特性 | `how-to/zh/user_guide.md`、`how-to/zh/web_guide.md` |
| 配置指南 | 是否新增/修改了配置参数 | `reference/zh/configuration_guide.md` |
| CLI 参考 | 是否新增/修改了命令行参数 | `reference/zh/cli_reference.md` |
| 硬件指南 | 是否有新的硬件适配信息 | `reference/zh/hardware_guide.md` |
| FAQ | 是否有新的常见问题 | `how-to/zh/faq.md` |

**其他**：

| 文档 | 检查条件 | 路径 |
| :--- | :--- | :--- |
| 技术决策 (ADR) | 是否做出重大技术选型决策 | `docs/explanation/zh/decisions/*.md` |
| 评估报告 | 是否完成代码质量评估 | `docs/explanation/zh/analysis/*.md` |
| 实施计划 | 是否有计划状态变更 | `docs/project/plans/*/IMPLEMENTATION_PLAN.md` |

**收尾**：中英双份象限（`tutorials/`、`how-to/`、`reference/`、`explanation/`）需同步 `zh/` 与 `en/`；文档变更使用独立 commit（`docs: update {文档名} for {原因}`）；最后刷新索引与机器地图：

```bash
python3 scripts/doc/generate-llms-txt.py --root docs --output docs/llms.txt
python3 scripts/doc/check-doc-links.py --root docs   # 零断链门禁
```
