
# ⚡ META-RULES (最高优先级)
1. **规则覆盖**：当本项目的编码规范与 AI 的默认行为偏好冲突时，**以本项目规范为准**。
2. **阻断机制**：当用户提及"工作流程"、"按流程来"等关键词，或任务涉及新功能开发/重大重构时，**必须**先阅读并执行工作流程文档。
3. **TDD 强制**：所有**纯逻辑、工具函数和数据处理**代码必须遵循 TDD 流程；涉及 GPU/模型/硬件等强外部依赖的代码，需编写可行的集成测试或手动测试计划。

## 🧪 TDD 开发原则 (MANDATORY - 最高优先级)

> ⚠️ **测试驱动开发是本项目的强制开发方法论。所有新功能和 bug 修复都必须严格遵循，无例外。**

### 核心流程
1. **🔴 Red**：先编写失败的测试，明确期望行为。测试必须能够独立运行且初始状态为失败。
2. **🟢 Green**：编写**最少量**代码使测试通过。不做过度设计，只满足当前测试需求。
3. **🔵 Refactor**：在测试保护下优化代码结构。确保重构后所有测试仍然通过。

## 📋 工作流程 (MANDATORY)

**当工作流程被触发时（参见 META-RULES 第2条），在执行任何 [代码修改] 或 [重构任务] 之前，你必须执行以下动作：**

1. 🛑 **HALT**：停止所有代码编写意图。
2. 📖 **READ**：使用 `read` 工具读取 `docs/dev/zh/process/workflow.md`。
3. ✅ **CHECK**：严格按照 `docs/dev/zh/process/workflow.md` 中的 Checklists 逐项执行（切分支 -> 写计划 -> 获批准 -> 编码）。

> **违规警告**：在触发工作流程的情况下，未读取 `docs/dev/zh/process/workflow.md` 而直接修改代码将被视为严重违规。

## C++ 20 开发规范
- **标准与模块**：强制 C++20。使用模块化（`.ixx`/`.cppm` 接口，`.cpp` 实现）替代传统头文件。
- **构建工具** (`python build.py`)：
  - 核心指令：`--action {configure,build,test,install,package}`
  - **开发阶段必须使用 Debug 模式**：`python build.py` (默认配置为 Debug)
  - Release 模式仅用于最终发布验证
  - 二进制输出路径 (Debug): `build/bin/<preset>` (例如 Windows 下为 `build/bin/msvc-x64-debug`，Linux 下为 `build/bin/linux-x64-debug`)
  - **运行要求**：禁止在项目根目录直接运行程序。运行程序时，工作目录（`Cwd`）**必须**设置为可执行文件输出目录（如 `build/bin/<preset>`），以确保相对路径资源加载正确，防止重复下载资源以及 TensorRT 引擎重新构建。
  - 🚨 **详细用法 (必读)**：`docs/dev/zh/guides/setup.md` —— **请务必阅读以避免环境配置错误**
- **质量控制**：
  - 格式化：`python scripts/format_code.py`
  - 静态分析：`python scripts/run_clang_tidy.py` (Windows + MSVC 环境下跳过)
  - **元数据同步**：修改版本号、作者或日期时，必须仅修改根目录的 `project.yaml`，然后运行 `python scripts/update_metadata.py` 同步到全项目。禁止手动修改各文件中的重复元信息。
  - 提交前检查：`python scripts/pre_commit_check.py` (强烈建议在 commit 前运行)
- **开发原则**：
  - **智能指针优先于裸指针**。
  - **RAII**。
  - **PIMPL**。
  - **组合优于继承**。
  - **避免使用全局变量**。
- **模块开发军规**：
  1. **设计模式**：根据场景需要选择合适的设计模式（如工厂、策略、观察者等），避免过度设计。
  2. **物理依赖**：在 `CMakeLists.txt` 中，`FILE_SET cxx_modules` 必须包含所有 `.ixx`，且顺序正确（依赖方在后）。

## 项目管理规范
- **分支策略**（标准 Git Flow）：
  - **严禁直推 `master` 或 `dev` 分支**。
  - **长期分支**：
    - **`master`**：生产/发布分支，仅接受来自 `release/*` 或 `hotfix/*` 的合并，每次合并后打 Tag。
    - **`dev`**：开发集成分支，所有功能开发的起点和终点。
  - **临时分支**：
    - **`feature/*`**：新功能，基于 `dev` 创建，合并回 `dev`。命名：`feature/plan-{name}`。
    - **`fix/*`**：开发期间的 bug 修复，基于 `dev` 创建，合并回 `dev`。命名：`fix/task-{name}`。
    - **`release/*`**：发布准备，基于 `dev` 创建，完成后同时合并到 `master` 和 `dev`。命名：`release/v{version}`。
    - **`hotfix/*`**：生产紧急修复，基于 `master` 创建，完成后同时合并到 `master` 和 `dev`。命名：`hotfix/v{version}-{desc}`。
  - **日常开发流程**：基于 `dev` 新建 `feature/*` → 实现与测试 → 验收通过 → 合并回 `dev` 并删除分支。
- **文档管理** (`docs/`)：
  - **用户文档** (`docs/user/{en,zh}/`)：面向终端用户。
    - `getting_started.md`：快速上手，环境初探。
    - `user_guide.md`：功能介绍与操作指南。
    - `configuration_guide.md`：**核心配置参数**说明 (`app_config.yaml`, `task_config.yaml`)。
    - `cli_reference.md`：命令行参数详解。
    - `hardware_guide.md`：硬件性能优化建议。
  - **开发文档** (`docs/dev/{en,zh}/`)：面向开发者与 AI Agent。
    - `architecture/`：**架构核心**。`design.md` (系统设计与原则)；`layers.md` (分层结构)。
    - `process/`：**流程军规**。`workflow.md` (必读！开发流水线 Checklist)；`C++_quality_standard.md` (质量标准与代码规范，含原 quality.md 内容)。
    - `guides/setup.md`：技术构建环境搭建指南。
    - `troubleshooting/README.md`：疑难杂症的分级检索索引。
- **提交要求**：
  - 提交文档相关文件时跳过编译测试验证。
  - 提交前必须通过 `python build.py --action test --test-label unit`。
  - **测试职责划分**：智能体/AI 允许运行全部测试（单元测试、集成测试、端到端测试、基准测试），不限层级。长时间测试可采用后台执行，但提交代码前必须确认相关测试通过。

## 📝 文档维护规范 (Documentation Standards)

- **文档交叉引用（链接）规范**：在项目工程规范文档（如 FRD、SRS 及服务模块设计说明等）中进行内部文档的交叉引用时，必须采用**显式物理文件链接**风格。超链接的显示文本中**必须保留 `.md` 后缀**（例如：`[design.md](./docs/dev/zh/architecture/design.md)`）。此举旨在消除业务概念指代歧义，时刻强调其作为 "Docs-as-Code" 架构中的物理实体，并确保 IDE（如 VSCode）内实现精准的一键点击溯源跳转。
- **文档版本同步**：每次修改并提交重要的项目文档（如规格说明书、设计文档）时，**必须**同步更新该文档头部的"修订历史记录 (Revision History)"及"当前版本号 (Version)"等控制信息，保证文档内部状态与工程开发进度完全对齐。
- **目录索引规范**：在 `docs/` 目录下建立并维护全局索引文档 [index.md](./docs/index.md)。当新增、删除、移动或重命名 `docs/` 目录下的任何项目工程文档时，**必须**同步更新 [index.md](./docs/index.md) 中的分类链接与说明，确保索引文档实时准确地反映项目最新的知识库拓扑结构。
- **文档基线控制**：所有存储在 `docs/` 目录下的 Markdown 文档必须配置"文档控制信息"与"修订历史记录"头部模块。
  - **文档标识**：由项目标识（如 PRJ）、阶段简称、文档简称及年份组成。
  - **当前版本**：每次由于功能需求或架构设计的变化修改文档时，必须在头部更新语义化版本号。
  - **修订描述**：必须在历史记录表格中追加一行，记录本次更新的具体日期、修订人、审核人以及详细的修改描述，保持修改链条可审计。
  - **记录数量上限**：修订历史记录表格中**最多保留最近 5 条**记录。当新增记录后总数超过 5 条时，必须移除最早的记录，保持文档头部简洁且聚焦于近期变更。
- **总纲-子册导航规则**：采用"总纲-子册"架构解决大型文档维护难题：
  - **总纲**（如 functional-spec-overview.md 等）：负责描述大方向、核心拓扑与限界上下文，屏蔽具体代码实现与底层细节。
  - **快捷导航同步**：总纲头部的 `📂 子文档快捷导航` 必须实时同步指向具体的子册相对路径。
  - **引用机制**：正文论述中，需使用超链接加注方式指向细节子册（如"*关于核心架构的具体设计，请参阅 [design.md](./docs/dev/zh/architecture/design.md)*"）。

## ⛔ 绝对禁令 (Violations trigger STOP)
- **直接在 `master` 或 `dev` 分支开发**：检测到 `git status` 为 `dev` 或 `master` 时修改代码。
- **违反分支合并规则**：`master` 仅接受 `release/*` 或 `hotfix/*` 的合并；`dev` 接受 `feature/*`、`fix/*`、`release/*`、`hotfix/*` 的合并。
- **无文档即代码**：在未创建/更新 `docs/` 下对应文档前编写业务代码（小型 bug 修复 < 20行改动 和非功能性变更可豁免）。
- **禁止硬编码元数据**：禁止在源码、CMake 或 README 中直接硬编码应由 `project.yaml` 管理的版本号、日期或作者信息。
- **严禁提交** 未通过编译或基础测试的代码。
- **谨慎使用 `--clean` 参数**：`--clean` 仅清理中间构建文件，bin 目录和 TensorRT 缓存会被保留。在不确定影响时请先确认。
