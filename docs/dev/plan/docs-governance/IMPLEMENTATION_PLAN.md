# 实施计划：文档治理 Diátaxis 标准化迁移

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-DOCGOV-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 已批准 (Approved)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 依据 `doc-governance` 技能与审计基线（健康度 37.2/100）制定治理计划；登记 DEC-1~DEC-7 裁决台账、六象限目标拓扑、67 篇文档完整映射表与 P0~P4 分批执行方案。用户已批准全量推进。 |

---

## 1. 背景与目标

### 1.1 触发背景

用户要求按 `doc-governance` 技能（`.agents/skills/doc-governance/`）对本项目知识库执行**严格治理**。该技能提供 Diátaxis 四象限拓扑、RFC 孵化流转、Docs-as-Code 自动化门禁与 5 条修订窗口自愈四项能力，并自带 5 个可执行脚本。

### 1.2 审计基线（`audit-doc-health.py --root docs` 实测）

| 维度 | 得分 | 实测问题 |
| :--- | :--- | :--- |
| 物理链接与断链防护 | **0 / 35** | 41 处断链，涉及 18 个文件 |
| 控制信息元数据基线 | **0.4 / 25** | 合规仅 1 / 67 篇 |
| 修订历史滑动窗口 | 18.8 / 20 | 4 篇超 5 条 |
| 孤儿文档引用度 | 8.1 / 10 | 13 篇未被索引引用 |
| Diátaxis 架构规范 | 10 / 10 | 六象限全部 0 篇 |
| **总分** | **37.2 / 100** | 通过门槛 80，当前 REJECTED |

**两项根因诊断（决定治理策略）**：

1. **41 处断链中多数为模板占位符误报**：27 处 404 里 20+ 处来自 `docs/dev/templates/` 与 `docs/dev/plan/*/IMPLEMENTATION_PLAN.md` 中的范例链接（`../plan/{name}/task/C++_task_{title}.md` 等未实例化占位符），**并非内容损坏**。
2. **控制头 66/67 不合规属判定不兼容**：审计脚本以**键名精确匹配**判定（`audit-doc-health.py:138-139`，`k in meta`），而项目控制头键名为 `文档标识 (Document ID)`、`当前版本 (Version)`（含英文括号后缀），永不匹配 → 需在工具侧放宽，而非改写 67 篇文档。

**其他超限项**：`docs/dev/zh/architecture/design.md` **1052 行**（技能红线 ≤800 行）；4 篇修订历史超标（`web_ui_design.md`、`architecture-performance-review.md`、`web_guide.md` zh/en）。

### 1.3 治理目标

1. `audit-doc-health.py` **≥ 80 分**且零断链；
2. 知识库拓扑对齐 Diátaxis 六象限（`proposals/`、`tutorials/`、`how-to/`、`reference/`、`explanation/`、`project/`）；
3. 门禁工具链入库并可在 CI 执行；
4. 规范源（`AGENTS.md`、`workflow.md`）与技能规程保持一致，消除双标。

---

## 2. 术语表（术语先行）

| 术语 | 定义 |
| :--- | :--- |
| **象限 (Quadrant)** | Diátaxis 四类读者心智：Tutorials（学习导向）、How-To（任务导向）、Reference（信息导向）、Explanation（理解导向） |
| **孵化层 (Proposals)** | RFC 提案单文件集中对齐空间，定案后结晶下沉至四象限 |
| **结晶下沉 (Crystallization)** | 提案获批后，将契约/规则/架构解构沉淀到对应稳态象限的过程 |
| **门禁 (Gate)** | 可执行、可失败、可 CI 化的校验（断链扫描、修订窗口、健康度评分） |
| **控制头 (Control Header)** | 文档头部的引用块元数据，含文档标识与当前版本 |
| **滑动窗口 (Sliding Window)** | 修订历史仅保留最近 5 条的裁剪规则 |
| **孤儿文档 (Orphan)** | 未被任何其他文档链接引用的文档 |

---

## 3. 裁决台账（已决项，禁止重开）

| ID | 裁决项 | 决定 | 被排除选项与依据 |
| :--- | :--- | :--- | :--- |
| **DEC-1** | 目录拓扑 | **渐进迁移至 Diátaxis 六象限** | 排除「全量一次性迁移」：技能迁移指南明令「拒绝暴力休克重构」；排除「只修可修项」：六象限合规度将永远为 0 |
| **DEC-2** | 双语布局 | **`<象限>/zh/` 与 `<象限>/en/`** | 排除 `xxx.en.md` 后缀：目录内语言混杂，破坏可读性；排除合并英文文档：破坏既有用户文档完整性 |
| **DEC-3** | 控制头对齐 | **保留既有格式，工具侧放宽为子串匹配** | 排除改写 67 篇文档格式：高机械风险且无内容收益 |
| **DEC-4** | 工具链落位 | **复制脚本至 `scripts/doc/`（受版本控制）** | 排除直接引用 `.agents/`：该目录在 `.gitignore` 中，CI 无法执行门禁 |
| **DEC-5** | 存量文件重命名 | **不改名，仅迁移路径** | 排除顺带 kebab-case 重命名：技能规程 §2.2 明令「切忌盲目发起大规模重命名」，且会放大断链面 |
| **DEC-6** | `plan/` 归属 | **迁移至 `docs/project/plans/`** | 技能拓扑无对应象限，按「工程演进与项目管理」收敛 |
| **DEC-7** | `templates/` 归属 | **迁移至 `docs/templates/`** | 模板非读者文档，不污染四象限 |

**本次新增决定**：

| ID | 决定 | 依据 |
| :--- | :--- | :--- |
| **DEC-8** | 已废止文档 `dev/en/process/quality.md` 迁入 `docs/archived/` | Hindsight 记忆页记录 `963b89b` 已合并 `quality.md` 至 `C++_quality_standard.md` 并宣告其失效；审计脚本亦以目录名 `archived` 作为归档标识（`audit-doc-health.py:140`） |
| **DEC-9** | 模板内占位符链接改用行内代码表示，不作为超链接 | 消除 20+ 处结构性误报，同时保留范例可读性 |

---

## 4. 目标拓扑

```text
docs/
├── index.md                 # 全局人类入口（保留并重写）
├── llms.txt                 # AI 智能体机器地图（生成）
├── GOVERNANCE.md            # 知识库治理规程
├── proposals/               # 需求与设计孵化层
│   └── README.md
├── tutorials/{zh,en}/       # 教程象限
├── how-to/{zh,en}/          # 操作指南象限（含 troubleshooting/）
├── reference/{zh,en}/       # 技术参考象限（含 rules/）
├── explanation/{zh,en}/     # 深度剖析象限（architecture/、analysis/、decisions/）
├── project/                 # 工程演进与项目管理
│   ├── plans/               # 历史计划与任务归档（来自 dev/plan/）
│   ├── changelog.md         # 指向根 CHANGELOG.md 的索引页
│   ├── roadmap.md
│   └── backlog.md
├── templates/               # 文档模板（来自 dev/templates/）
└── archived/                # 已废止文档
```

---

## 5. 完整迁移映射表（67 篇）

### 5.1 教程象限 `tutorials/`

| 现路径 | 目标路径 |
| :--- | :--- |
| `user/zh/getting_started.md` | `tutorials/zh/getting_started.md` |
| `user/en/getting_started.md` | `tutorials/en/getting_started.md` |

### 5.2 操作指南象限 `how-to/`

| 现路径 | 目标路径 |
| :--- | :--- |
| `user/zh/user_guide.md`、`user/zh/web_guide.md`、`user/zh/faq.md` | `how-to/zh/` 同名 |
| `user/en/user_guide.md`、`user/en/web_guide.md`、`user/en/faq.md` | `how-to/en/` 同名 |
| `dev/zh/guides/build.md`、`dev/zh/guides/setup.md` | `how-to/zh/` 同名 |
| `dev/en/guides/setup.md` | `how-to/en/setup.md` |
| `dev/zh/process/vcpkg_best_practice.md`、`dev/zh/process/CI_CD_recommendations.md` | `how-to/zh/` 同名 |
| `dev/{zh,en}/troubleshooting/**` | `how-to/{zh,en}/troubleshooting/**` |

### 5.3 技术参考象限 `reference/`

| 现路径 | 目标路径 |
| :--- | :--- |
| `user/zh/configuration_guide.md`、`cli_reference.md`、`hardware_guide.md` | `reference/zh/` 同名 |
| `user/en/configuration_guide.md`、`cli_reference.md`、`hardware_guide.md` | `reference/en/` 同名 |
| `dev/zh/process/workflow.md`、`C++_quality_standard.md` | `reference/zh/rules/` 同名 |
| `dev/en/process/quality.md` | `archived/quality.md`（见 DEC-8） |

### 5.4 深度剖析象限 `explanation/`

| 现路径 | 目标路径 |
| :--- | :--- |
| `dev/zh/architecture/{design,layers,web_ui_design}.md` | `explanation/zh/architecture/` 同名 |
| `dev/en/architecture/{design,layers,web_ui_design}.md` | `explanation/en/architecture/` 同名 |
| `dev/zh/evaluation/architecture-performance-review.md`、`web-ui-design-completion-review.md`、`vulkan-backend-feasibility-review.md` | `explanation/zh/analysis/` 同名 |
| `dev/zh/guides/compiler_selection.md` | `explanation/zh/decisions/compiler_selection.md` |

> 📌 范围外：`compiler_selection.md` 的 **MADR 3.0 格式重构**（重编号为 `0001-xxx.md`）不在本次范围，登记至 `project/backlog.md`。

### 5.5 工程演进象限 `project/`

| 现路径 | 目标路径 |
| :--- | :--- |
| `dev/plan/{perf-p0,perf-p1a,perf-p1b,perf-p2minor,web-robustness}/**` | `project/plans/{同名}/**` |
| 本次治理计划（新增） | `project/plans/docs-governance/IMPLEMENTATION_PLAN.md` |
| 仓库根 `CHANGELOG.md` | **保持原位**（`.github/workflows/release.yml` 依赖），新增 `project/changelog.md` 索引页指向 |

### 5.6 其他

| 现路径 | 目标路径 |
| :--- | :--- |
| `dev/templates/C++_{evaluation,plan,task}_template.md` | `templates/` 同名（DEC-7） |
| `index.md` | `index.md`（重写为六象限导航） |
| — | 新增 `llms.txt`、`GOVERNANCE.md`、`proposals/README.md`、`project/roadmap.md`、`project/backlog.md` |

---

## 6. 分批执行计划

| 批次 | 内容 | 验收标准 |
| :--- | :--- | :--- |
| **P0 自愈** | ① `scripts/doc/` 工具落位 + 控制头子串匹配适配 + 模板占位符豁免；② 修订历史裁剪 4 篇；③ 模板占位符链接改行内代码；④ 真实断链修复（锚点/路径）；⑤ 13 篇孤儿文档收录 | `check-doc-links.py` exit 0；`trim-revision.py` 零告警 |
| **P1 立骨架** | 六象限目录 + `GOVERNANCE.md` + `llms.txt` + `proposals/README.md` + `project/{roadmap,backlog,changelog}.md` | 象限拓扑分布不再为 0；`audit-doc-health.py` 分数上升 |
| **P2 迁移（5 批）** | 按象限顺序：`explanation/` → `how-to/` → `reference/` → `tutorials/` → `project/`（含 `templates/`），每批 `git mv` + 全库链接重写 + 门禁验证 | 每批 `check-doc-links.py` exit 0；`git status` 无残留空目录；`git mv` 保留历史 |
| **P3 拆巨石** | `design.md`（1052 行）拆为总纲 + 子册 | 单文档 ≤ 800 行；总纲-子册导航互链有效 |
| **P4 规范同步** | `AGENTS.md` 文档规范章节、`workflow.md` 路径约定、CI 门禁接入 | 规范与技能零冲突；CI 可执行文档门禁 |

**提交策略**：每批次独立原子提交，message 采用 `docs(governance): <scope>` 前缀；纯文档变更按 `workflow.md` 第 9 条跳过测试。

---

## 7. 门禁与验收

```bash
# 断链门禁（必须 exit 0）
python3 scripts/doc/check-doc-links.py --root docs
# 修订窗口门禁
python3 scripts/doc/trim-revision.py --root docs
# 健康度总门禁（最终必须 >= 80）
python3 scripts/doc/audit-doc-health.py --root docs
# 机器地图刷新
python3 scripts/doc/generate-llms-txt.py --root docs --output docs/llms.txt
```

---

## 8. 风险与回滚

| 风险 | 等级 | 缓解 |
| :--- | :--- | :--- |
| 迁移过程中产生新断链 | **P0** | 每批迁移后立即运行断链门禁；未通过不得提交 |
| Git 历史断裂 | P1 | 全程使用 `git mv`，严禁删除后新建 |
| 分批提交中间态不可用 | P1 | 每批自洽可验证；分批提交保证可单独 revert |
| 工具适配引入误判（如豁免过宽） | P1 | 适配逻辑限定于 `templates/` 与占位符模式，并在 P0 用全量扫描对比验证 |
| 与未合并的 `feature/eval-vulkan-support` 冲突 | P2 | 本分支基于该分支切出，评估文档已包含在迁移范围内 |

**回滚**：任一批次验收失败即 `git reset --hard` 该批次提交（**需用户确认**），重新分析；已提交批次间互不依赖，可逐批 revert。

---

## 9. 范围外事项（明确不做）

1. **文件重命名**（DEC-5a）——保留既有命名；
2. **MADR 3.0 重构** `compiler_selection.md`——登记 backlog；
3. **RFC 补写**：本次治理已经用户批准，不再倒补 RFC 流程；
4. **英文文档翻译补齐**：仅迁移现有英文文档，不新增翻译；
5. **根目录 `CHANGELOG.md` 迁移**——CI 依赖，保持原位。
