# 脚本说明文档

本文档详细说明了 `scripts` 目录下各个脚本的功能、使用方法以及已知问题。

## 脚本列表

### 1. format_code.ps1

*   **功能**: 使用 `clang-format` 格式化项目中的 C++ 源代码文件。
*   **依赖**: 需要系统路径中包含 `clang-format` 可执行文件。
*   **配置**: 使用项目根目录下的 `.clang-format` 配置文件。
*   **用法**:
    ```powershell
    ./scripts/format_code.ps1
    ```

### 2. run_clang_tidy.ps1

*   **功能**: 对项目源代码运行 `clang-tidy` 静态分析。
*   **依赖**:
    *   需要系统路径中包含 `clang-tidy`。
    *   需要项目已构建并生成 `compile_commands.json`（通常在 `build/msvc-x64-debug` 等目录下）。
*   **配置**: 使用项目根目录下的 `.clang-tidy` 配置文件。
*   **用法**:
    ```powershell
    ./scripts/run_clang_tidy.ps1
    ```
*   **已知问题 (C++20 模块)**:
    *   **现象**: 运行脚本时会报告大量 `error: module 'xxx' not found`。
    *   **原因**: 本项目使用 MSVC 编译器构建，生成的模块接口文件格式为 `.ifc`。目前的 `clang-tidy`（基于 LLVM）不兼容 MSVC 的 `.ifc` 二进制格式，因此无法解析模块导入。
    *   **影响**: 涉及模块导入的代码可能无法进行深入的语义分析，但命名规范、代码风格等基于 AST 的检查仍然有效。可以忽略模块未找到的报错。

### 3. pre_commit_check.ps1

*   **功能**: Git 提交前的钩子脚本，用于自动化检查。
    *   检查暂存区（Staged）的文件。
    *   对暂存的 C++ 文件运行 `clang-format` 并自动重新暂存更改。
    *   对暂存的文件运行 `clang-tidy` 进行静态分析（如果配置了构建目录）。
*   **用法**: 通常由 Git 自动调用，也可以手动运行测试。

### 4. install_hooks.ps1

*   **功能**: 将 `scripts/git_hooks` 目录下的钩子安装到 `.git/hooks` 目录中。
*   **用法**:
    ```powershell
    ./scripts/install_hooks.ps1
    ```

### 5. set_msvc_compiler_env.ps1

*   **功能**: 设置 MSVC 编译器的环境变量，以便在命令行中使用 `cl.exe` 等工具。
*   **用法**:
    ```powershell
    ./scripts/set_msvc_compiler_env.ps1
    ```

## 常见问题

### Clang-Tidy 与 MSVC 模块兼容性

如上所述，在使用 MSVC 构建 C++20 模块项目时，`clang-tidy` 无法正确读取 `.ifc` 模块文件。这是工具链互操作性的限制。

**规避方案**:
目前没有完美的规避方案。建议关注非模块相关的代码质量警告（如命名风格、性能建议等），并忽略模块导入错误。

### 脚本执行权限

如果在 PowerShell 中运行脚本遇到权限问题，请使用以下命令运行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/xxx.ps1
```
