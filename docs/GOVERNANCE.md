# 知识库治理规程 (Documentation Governance)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DOC-GOVERNANCE-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 依据 `doc-governance` 技能初始化知识库治理规程：确立 Diátaxis 六象限拓扑、文档控制头与 5 条修订窗口基线、显式物理链接规范、生命周期流转与自动化门禁。 |

---

## 1. 目的与适用范围

本规程是 `docs/` 知识库的**单一治理真相源（Single Source of Truth）**，适用于本仓库内所有 Markdown 文档的新增、修改、迁移与归档。

**设计原则**：

1. **读者心智优先（Diátaxis）**：按"读者要做什么"划分象限，严禁职责混杂；
2. **Docs-as-Code**：治理规则必须可执行、可失败、可 CI 化，禁止只靠人工自觉；
3. **渐进演进**：结构迁移按象限分批进行，每批零断链后才提交，拒绝暴力重构；
4. **Git 历史可审计**：移动文档一律使用 `git mv`，严禁删除后新建。

---

## 2. 目录拓扑与象限职责

```text
docs/
├── index.md                 # 人类总入口：全景导航
├── llms.txt                 # AI 智能体机器地图（自动生成，勿手改）
├── GOVERNANCE.md            # 本规程
├── proposals/               # 💡 孵化层：RFC 提案集中对齐，定案后结晶下沉
├── tutorials/               # 🎓 学习导向：新手 5 分钟上手
├── how-to/                  # 🛠️ 任务导向：完成具体任务的步骤
├── reference/               # 📖 信息导向：契约、配置、规则等机械事实
├── explanation/             # 💡 理解导向：架构设计与"为什么"
├── project/                 # 🚀 工程演进：roadmap / backlog / changelog / plans
├── templates/               # 文档模板
└── archived/                # 已废止文档（保留可追溯性）
```

**象限职责与排他规则**：

| 象限 | 回答的问题 | 必须放 | 禁止放 |
| :--- | :--- | :--- | :--- |
| `tutorials/` | "我该怎么开始？" | 端到端首次成功路径 | 完整参数清单、架构权衡 |
| `how-to/` | "怎么完成 X？" | 可执行步骤、排障手册 | 长篇原理推演 |
| `reference/` | "X 的取值/契约是什么？" | 配置项、API、状态机、规则 | 教学、设计动机 |
| `explanation/` | "为什么这样设计？" | 架构总纲、评估分析、ADR | 操作步骤 |
| `proposals/` | "我们打算做什么？" | RFC 草案、未定案设想 | 已生效的权威契约 |
| `project/` | "项目进度与待办？" | 路线图、待办、变更日志、历史计划 | 技术设计正文 |

**双语布局**：需要中英双份的象限采用 `<象限>/zh/` 与 `<象限>/en/` 子目录；单语内容直接置于象限根。

---

## 3. 文档控制头规范

所有文档**必须**包含控制头与修订历史两个头部模块：

```markdown
> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-<域>-<阶段>-<文档>-<年份>
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 草案 (Draft) | 评审中 (In Review) | 正式 (Official) | 已批准 (Approved)
> - **权威性 (Authority)**: Normative | Informative
> - **所有者 (Owner)**: <姓名>
> - **审核人 (Reviewer)**: <姓名>
> - **最后更新 (Last Updated)**: YYYY-MM-DD

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | YYYY-MM-DD | <修订人> | <审核人> | <变更描述> |
```

**语义化版本**：内容结构调整或契约变更升次版本号；措辞润色升修订号；首次发布为 V1.0.0。

**权威性取值**：`Normative`（必须遵守，如本规程、工作流程）与 `Informative`（参考性，如评估报告）。

---

## 4. 修订历史滑动窗口

修订历史表格**最多保留最近 5 条**，超出必须裁剪。

- 自动裁剪：`python3 scripts/doc/trim-revision.py --root docs --fix`
- 只读检查：`python3 scripts/doc/trim-revision.py --root docs`
- 裁剪掉的历史通过 `git log --oneline <file>` 追查。

---

## 5. 链接与交叉引用规范

1. **显式物理链接**：文档间引用必须使用相对路径超链接，且**显示文本保留 `.md` 后缀**，例如 `[design.md](../architecture/design.md)`；
2. **禁止裸路由**：严禁无后缀引用（如 `[design](../architecture/design)`）或绝对物理路径；
3. **禁止未实例化占位符链接**：模板中的 `{name}` / `{title}` 等占位符必须以**行内代码**表示，不得写成超链接；
4. **锚点必须真实存在**：锚点按 GitHub 规则生成（保留 `_`、每个空格生成一个 `-`），禁止引用不存在的章节或源码行号（`#L155` 形式不适用于 Markdown 文档）；
5. **指向目录的链接**：目标目录必须包含 `index.md` 或 `README.md`，否则改用行内代码表示路径。

---

## 6. 文件命名与语言规范

- **命名**：遵循既有工程习惯，新增文档优先使用**全小写 kebab-case**（如 `vulkan-backend-feasibility-review.md`）；存量文件**不做批量重命名**，以免断链与 Git 历史断裂；
- **语言继承**：中文为主导的项目，正文与解释统一使用中文；代码标识符、API 路径、JSON 键名、Commit 前缀保持英文，严禁硬译；
- **正交分离**：背景、权衡、步骤使用中文；机械标识符使用英文，禁止中英混杂成句。

---

## 7. 生命周期：提案孵化与结晶下沉

1. **孵化**：新功能/架构设想先在 `proposals/` 以单份 RFC 集中对齐（草案 → 评审 → 接受）；
2. **结晶下沉**：方案获批后，将契约沉入 `reference/`、架构设计沉入 `explanation/architecture/`、操作步骤沉入 `how-to/`；
3. **归档防腐**：原 RFC 标记为 `Implemented` 并移入 `proposals/archive/`；
4. **未定案内容**严禁写入 `reference/`（防止草案污染权威真相源）。

---

## 8. 自动化门禁

工具链位于 `scripts/doc/`（受版本控制，可进 CI）：

| 脚本 | 用途 | 命令 | 通过标准 |
| :--- | :--- | :--- | :--- |
| `check-doc-links.py` | 断链与锚点静态审计 | `python3 scripts/doc/check-doc-links.py --root docs` | Exit 0，零断链 |
| `trim-revision.py` | 修订历史窗口审计/自愈 | `python3 scripts/doc/trim-revision.py --root docs [--fix]` | 零超标 |
| `audit-doc-health.py` | 综合健康度体检 | `python3 scripts/doc/audit-doc-health.py --root docs` | ≥ 80 分 |
| `generate-llms-txt.py` | 生成 AI 机器地图 | `python3 scripts/doc/generate-llms-txt.py --root docs --output docs/llms.txt` | 成功生成 |
| `scaffold-doc.sh` | 按模板创建新文档 | `bash scripts/doc/scaffold-doc.sh <type> <name>` | 生成合规控制头 |

**触发时机**：新增/移动/删除文档后必须运行断链门禁；版本发布前运行健康度体检。

---

## 9. 豁免与例外

1. `docs/archived/` 下的废止文档在兼容模式下豁免控制头检查，但**必须**保留原控制头与废止说明；
2. 仓库根目录 `CHANGELOG.md` 由 `.github/workflows/release.yml` 消费，保持原位，不在 `docs/` 内重复维护；
3. `llms.txt` 为自动生成产物，人工修改会在下次生成时被覆盖。

---

## 10. 相关文档

- [index.md](index.md) —— 知识库全景导航
- [workflow.md](reference/zh/rules/workflow.md) —— 开发流水线 Checklist
- [AGENTS.md](../AGENTS.md) —— 项目级文档维护规范
