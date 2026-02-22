
# ⚡ META-RULES (最高优先级)
1. **规则覆盖**：当本项目的编码规范与 AI 的默认行为偏好冲突时，**以本项目规范为准**。
2. **阻断机制**：当用户提及"工作流程"、"按流程来"等关键词，或任务涉及新功能开发/重大重构时，**必须**先阅读并执行工作流程文档。
3. **语言锁定**：无论用户使用何种语言提问，**必须** 使用中文进行思考和回答（代码注释除外）。
4. **TDD 强制**：所有**纯逻辑、工具函数和数据处理**代码必须遵循 TDD 流程；涉及 GPU/模型/硬件等强外部依赖的代码，需编写可行的集成测试或手动测试计划。

## 🧪 TDD 开发原则 (MANDATORY - 最高优先级)

> ⚠️ **测试驱动开发是本项目的强制开发方法论。所有新功能和 bug 修复都必须严格遵循，无例外。**

### 核心流程
1. **🔴 Red**：先编写失败的测试，明确期望行为。测试必须能够独立运行且初始状态为失败。
2. **🟢 Green**：编写**最少量**代码使测试通过。不做过度设计，只满足当前测试需求。
3. **🔵 Refactor**：在测试保护下优化代码结构。确保重构后所有测试仍然通过。

### TDD 禁令
- **严禁**：在没有对应测试的情况下编写纯逻辑/工具类新功能代码（涉及硬件依赖的代码需提供手动测试计划）。
- **严禁**：先写实现代码再补测试（事后补测试不是 TDD）。
- **严禁**：为了通过测试而修改测试本身（除非测试逻辑确实有误）。
- **严禁**：跳过 Refactor 阶段（技术债务的主要来源）。

### TDD 检查点
- 每个 PR/提交必须包含对应的测试代码。
- 测试覆盖率不得因新代码而下降。
- 测试必须是**行为测试**而非**实现测试**（测试"做什么"而非"怎么做"）。

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
    - `process/`：**流程军规**。`workflow.md` (必读！开发流水线 Checklist)；`quality.md` (质量标准)；`C++_quality_standard.md` (代码规范)。
    - `guides/setup.md`：技术构建环境搭建指南。
    - `troubleshooting/README.md`：疑难杂症的分级检索索引。
- **提交要求**：
  - 仅提交源码与必要资源，禁止提交文档、日志或构建文件(除非用户明确要求)。
  - 提交文档相关文件时跳过编译测试验证。
  - 文件重命名/移动必须使用 `git mv`。
  - 提交前必须通过 `python build.py --action test --test-label unit`。
  - **测试职责划分**：智能体/AI 仅需运行单元测试；集成测试和端到端测试由用户手动验证。

## ⛔ 绝对禁令 (Violations trigger STOP)
- **直接在 `master` 或 `dev` 分支开发**：检测到 `git status` 为 `dev` 或 `master` 时修改代码。
- **违反分支合并规则**：`master` 仅接受 `release/*` 或 `hotfix/*` 的合并；`dev` 接受 `feature/*`、`fix/*`、`release/*`、`hotfix/*` 的合并。
- **无文档即代码**：在未创建/更新 `docs/` 下对应文档前编写业务代码（小型 bug 修复 < 20行改动 和非功能性变更可豁免）。
- **无测试即提交**：在未运行 `python build.py --action test --test-label unit` 并截图/贴出日志前进行 git commit（文档/注释等非代码类变更及 `docs/` 目录下内容除外）。
- **幻觉引用**：引用不存在的文件路径（必须先 `ls` 或 `glob` 确认文件存在）。
- **禁止硬编码元数据**：禁止在源码、CMake 或 README 中直接硬编码应由 `project.yaml` 管理的版本号、日期或作者信息。
- **严禁递归删除** `build` 目录及其子文件。
- **严禁提交** 未通过编译或基础测试的代码。
- **谨慎使用 `--clean` 参数**：`--clean` 仅清理中间构建文件，bin 目录和 TensorRT 缓存会被保留。在不确定影响时请先确认。
