---
trigger: always_on
---

## 核心原则
- **对话语言**：强制使用中文。
- **外部引用**：按需加载 `@` 前缀标记的文档，支持递归引用。

## C++ 20 开发规范
- **标准与模块**：强制 C++20。使用模块化（`.ixx`/`.cppm` 接口，`.cpp` 实现）替代传统头文件。
- **构建工具** (`python build.py`)：
  - 核心指令：`configure` | `build` | `test` | `install` | `package`
  - 详细用法参见：`@docs/build.md`
- **质量控制**：
  - 格式化：`python scripts/format_code.py`
  - 静态分析：`python scripts/run_clang_tidy.py` (Windows + MSVC 环境下跳过)
- **开发原则**：
  - **智能指针优先于裸指针**。
  - **RAII**。
  - **PIMPL**。
  - **组合优于继承**。
  - **避免使用全局变量**。

## 项目管理规范
- **分支策略**：
  - **严禁直推主分支**。
  - **流程**：基于主分支新建 -> 实现与测试 -> 验收通过 -> 合并主分支并删除。
  - **命名**：`feature/plan-{name}` 或 `fix/task-{name}`。
- **文档管理** (`/docs/dev_docs/`)：
  - 计划：`plan/{name}/C++_plan_{title}.md` (使用 `@docs/dev_docs/C++_plan_template.md`)
  - 任务：`plan/{name}/task/C++_task_{title}.md` (使用 `@docs/dev_docs/C++_task_template.md`)
  - 评估：`evaluation/C++_evaluation_{title}.md` (使用 `@docs/dev_docs/C++_evaluation_template.md`)
  - 记录：`C++_troubleshooting.md` (记录疑难杂症, 使用 `@docs/dev_docs/C++_troubleshooting.md`)
- **提交要求**：
  - 仅提交源码与必要资源，禁止提交文档、日志或构建文件。
  - 文件重命名/移动必须使用 `git mv`。
  - 提交前必须通过 `build.py test`。

## 禁止操作
- 严禁递归删除 `build` 目录及其子文件。
- 严禁提交未通过编译或基础测试的代码。

## 工作流程
详细流程参见：`@docs/dev_docs/workflow.md`