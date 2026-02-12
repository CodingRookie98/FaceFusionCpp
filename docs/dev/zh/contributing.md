# 贡献指南 (Contributing Guide)

感谢您对 FaceFusionCpp 的关注！我们欢迎错误报告、功能请求和代码贡献。

## 1. 行为准则 (Code of Conduct)
我们期望所有贡献者保持尊重和包容。任何骚扰或攻击行为都是不可容忍的。

---

## 2. 快速入门 (Getting Started)

1.  **Fork** 仓库到您的 GitHub 账户。
2.  **Clone** 到本地：
    ```bash
    git clone https://github.com/YOUR-USERNAME/faceFusionCpp.git
    cd faceFusionCpp
    ```
3.  **配置** 开发环境。请参阅 [构建指南](build_guide.md)。

---

## 3. 开发流程 (Development Workflow)

### 3.1 分支策略 (Branching)
*   **严禁** 直接推送到 `main` (或 `master`, `linux/dev`)。
*   基于您的工作类型创建新分支：
    *   新功能: `feature/short-description` (例如 `feature/add-video-support`)
    *   Bug 修复: `fix/issue-number-description` (例如 `fix/123-memory-leak`)

### 3.2 测试驱动开发 (TDD) - **强制执行**
我们严格遵循 TDD。您**必须**在实现功能之前编写测试。
1.  **Red**: 在 `tests/` 中编写一个失败的测试用例。
2.  **Green**: 编写最少量的代码使测试通过。
3.  **Refactor**: 在保持测试通过的前提下优化代码。

### 3.3 编码风格 (Style)
*   **C++ 标准**: C++20。
*   **格式**: 使用 `clang-format`。运行：
    ```bash
    python scripts/format_code.py
    ```
*   **检查**: 使用 `clang-tidy`。提交前确消除所有警告。

---

## 4. Pull Request 流程

1.  **同步**: 确保您的分支与上游 `main` 保持同步。
2.  **测试**: 在本地运行完整测试套件：
    ```bash
    python build.py --action test
    ```
    **包含失败测试的 PR 将被拒绝。**
3.  **提交**: 使用 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/) 规范：
    *   `feat: add face enhancer`
    *   `fix: resolve crash on exit`
    *   `docs: update readme`
    *   `refactor: optimize pipeline`
4.  **推送**: 将分支推送到您的 Fork 仓库。
5.  **创建 PR**: 在 GitHub 上发起 Pull Request。
    *   清晰描述您的更改。
    *   关联相关 Issue (例如 "Closes #123")。

---

## 5. 代码审查 (Review)
*   维护者将审查您的代码。
*   请根据反馈进行修改。
*   一旦通过并且 CI 成功，您的代码将被合并。

感谢您帮助 FaceFusionCpp 变得更好！
