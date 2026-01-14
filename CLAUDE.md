---
trigger: always_on
---

- 对话语言为中文

## 外部文件引用

当遇到文件引用（如 `@docs/xxx.md`）时，使用 Read 工具按需加载：
- 不要预先加载所有引用，按实际需要懒加载
- 必要时递归加载引用

## C++ 开发规范（C++20）

### 语言标准与模块化
- **标准版本**：采用 **C++20** 标准
- **模块系统**：使用 C++20 模块特性替代传统头文件
  - 模块接口文件（`.ixx`/`.cppm`）仅包含声明和导出
  - 模块实现文件（`.cpp`）包含具体实现

### 编译与构建

- **构建和测试**：通过运行 `python build.py` 脚本构建项目。
  - 配置：`python build.py --action configure`
  - 构建：`python build.py --action build`
  - 测试：`python build.py --action test`
  - 更多参数请运行 `python build.py --help`
- **代码格式化**：`python scripts/format_code.py`
- **静态分析**：`python scripts/run_clang_tidy.py`


### 文档模板

- C++计划文档模板：`@docs/dev_docs/plan_template.md`
- C++任务文档模板：`@docs/dev_docs/task_template.md`
- C++评估文档模板：`@docs/dev_docs/evaluation_template.md`

## 项目管理规范

### 1. C++计划文档

- **路径**：`@docs/dev_docs/plan/{plan_name}/C++_plan_{计划名称}.md`
- **格式**：中文 Markdown

### 2. C++任务文档

- **路径**：`@docs/dev_docs/plan/{plan_name}/task/C++_task_{task_name}.md`
- **格式**：中文 Markdown
- 每个任务应独立成文档

### 3. C++代码质量评估文档

- **路径**：`@docs/dev_docs/evaluation/C++_evaluation_{title}.md`
- **格式**：中文 Markdown

### 4. C++疑难杂症记录文档

- **路径**：`@docs/dev_docs/C++_troubleshooting.md`
- **格式**：中文 Markdown
- **说明**：记录开发中遇到的疑难问题及解决方案

## 禁止的操作

- 不要递归删除 build 目录及目录下的文件
- 禁止直接修改主分支的代码，只能通过创建新分支进行开发和测试。

### 工作流程
工作流程请参见文档：`@docs/dev_docs/workflow.md`

## 必须遵守的规范

- 移动文件或重命名文件等操作优先使用git命令，避免直接操作文件系统
- 代码要经过完全测试才能提交
- 所有开发工作应在其他分支上进行，如 feature/branch、fix/branch 等

## 其他事项

- 系统为windows且编译器为msvc时不要进行 clang-tidy 分析