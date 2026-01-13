# 使用 `build.ps1` 脚本配置和构建

本项目提供了 `build.ps1` 脚本，旨在简化配置、构建、测试和打包的流程。该脚本会自动检测系统环境（如最大处理器核心数），并支持 Debug 和 Release 两种配置。

## 1. 快速开始

在 PowerShell 中执行以下命令即可开始构建：

```powershell
# 默认行为：配置并构建 Debug 版本
.\build.ps1
```

## 2. 常用命令

```powershell
# 仅构建 Release 版本
.\build.ps1 -Configuration Release -Action build

# 运行测试
.\build.ps1 -Configuration Debug -Action test

# 安装项目
.\build.ps1 -Configuration Release -Action install

# 打包项目
.\build.ps1 -Configuration Release -Action package

# 仅构建主程序 (旧默认行为)
.\build.ps1 -Target FaceFusionCpp
```

## 3. 参数说明

脚本支持以下参数，用于定制构建行为：

| 参数 | 说明 | 可选值 | 默认值 |
| :--- | :--- | :--- | :--- |
| `-Configuration` | 构建配置类型 | `Debug`, `Release` | `Debug` |
| `-Action` | 执行的操作 | `configure` (仅配置)<br>`build` (仅构建)<br>`test` (构建并运行测试)<br>`install` (安装)<br>`package` (打包)<br>`both` (配置+构建) | `both` |
| `-Target` | 构建目标 | `all` 或具体目标名 | `all` |
| `-EnableCoverage` | 启用代码覆盖率 | `[switch]` | `False` |
| `-EnableStaticAnalysis` | 启用静态分析 | `[switch]` | `False` |
| `-SkipBuild` | 跳过自动构建 (仅对 test 有效) | `[switch]` | `False` |

> **注意**: 脚本会自动利用系统所有可用核心进行并行构建，无需手动指定 `-j` 参数。

## 4. 完整工作流示例

### 开发流程
```powershell
# 1. 首次构建 Debug 版本
.\build.ps1

# 2. 运行测试并查看结果
.\build.ps1 -Action test

# 3. (可选) 生成覆盖率报告
.\build.ps1 -Action test -EnableCoverage
```

### 发布流程
```powershell
# 1. 一键构建并测试 Release 版本
.\build.ps1 -Configuration Release -Action both
.\build.ps1 -Configuration Release -Action test

# 2. 生成安装包
.\build.ps1 -Configuration Release -Action package
```

## 5. 代码风格与静态检查

本项目使用 Clang 工具链进行代码格式化和静态分析。

### 代码格式化 (Clang-Format)

使用 `scripts/format_code.ps1` 脚本自动格式化代码。该脚本会根据 `.clang-format` 配置文件递归扫描 `src`, `facefusionCpp`, `tests` 目录下的 C++ 源文件。

```powershell
.\scripts\format_code.ps1
```

### 静态分析 (Clang-Tidy)

使用 `scripts/run_clang_tidy.ps1` 脚本运行静态分析。该脚本依赖于 CMake 配置生成的 `compile_commands.json`。

```powershell
# 1. 确保已配置项目 (生成 compile_commands.json)
.\build.ps1 -Action configure

# 2. 运行 Clang-Tidy 检查
.\scripts\run_clang_tidy.ps1
```

### 构建时静态分析

可以在 CMake 构建过程中启用 Clang-Tidy 检查。

```powershell
# 启用 Clang-Tidy 配置
cmake -DENABLE_CLANG_TIDY=ON --preset msvc-x64-debug

# 或者直接修改 CMakeLists.txt 中的 option 默认值
```

## 6. 输出目录

构建产物将位于 `build` 目录下：

- **Debug 构建**: `build/msvc-x64-debug/`
- **Release 构建**: `build/msvc-x64-release/`
- **安装目录**: 对应构建目录下的 install 文件夹（如未指定其他前缀）
- **打包文件**: `build/packages/` (通常) 或构建根目录

---

# 使用 CMake 直接构建

如果您更喜欢使用原生的 CMake 命令行工具，或者使用 IDE（如 VS Code, Visual Studio, CLion）内置的 CMake 支持，可以直接使用 CMake Presets。

## 1. 环境准备

在开始之前，请确保已满足以下条件：

- **CMake**: 3.25 或更高版本。
- **编译器**: Visual Studio 2022 (MSVC)。
- **构建系统**: Ninja (推荐) 或 Visual Studio 生成器。
- **环境设置**: 执行 CMake 命令前，必须处于 MSVC 开发环境中。
    - 方法一：使用 `Visual Studio 2022 Developer PowerShell`。
    - 方法二：运行项目提供的脚本：`.\scripts\set_msvc_compiler_env.ps1`。

## 2. 常用操作

项目利用 `CMakePresets.json` 预定义了常用的构建配置。

### 配置 (Configure)

```powershell
# 查看所有可用预设
cmake --list-presets

# 配置 Debug 版本
cmake --preset msvc-x64-debug

# 配置 Release 版本
cmake --preset msvc-x64-release
```

### 构建 (Build)

```powershell
# 构建 Debug 版本 (自动使用多线程)
cmake --build --preset msvc-x64-debug

# 构建 Release 版本 (自动使用多线程)
cmake --build --preset msvc-x64-release
```
> **提示**: 这里的 `--preset` 已经定义了构建参数。如果需要手动指定并发数，可以在命令末尾添加 `-j <核心数>`，例如 `cmake --build --preset msvc-x64-debug -- -j 8` (取决于生成器) 或直接 `cmake --build --preset msvc-x64-debug -j 8` (CMake 3.12+)。

### 测试 (Test)

```powershell
# 运行 Debug 测试
ctest --preset msvc-x64-debug

# 运行 Release 测试
ctest --preset msvc-x64-release
```

### 打包 (Package)

```powershell
# 进入构建目录进行打包
cd build/msvc-x64-release
cpack -C Release
```

## 3. 工作流 (Workflow)

CMake 3.25+ 引入了工作流预设，允许通过一条命令顺序执行 **配置 -> 构建 -> 测试 -> 打包** (如果配置了打包步骤)。

```powershell
# 执行完整的 Debug 工作流
cmake --workflow --preset msvc-x64-debug

# 执行完整的 Release 工作流
cmake --workflow --preset msvc-x64-release
```

---

## 常见问题 (FAQ)

### Q: 提示 `CMake executable not found`?
**A**: 请安装 CMake 并确保将其添加到系统 PATH 中。推荐安装位置：`C:\Program Files\CMake\bin\`。

### Q: 提示 `Visual Studio DevShell module not found`?
**A**: 请确保安装了 Visual Studio 2022 及其 C++桌面开发工作负载。如果安装位置非默认，请修改 `build.ps1` 脚本中的路径。

## 相关文档

- [CMake Documentation](https://cmake.org/documentation/)
- [CPack Documentation](https://cmake.org/cmake/help/latest/manual/cpack.1.html)
- [CTest Documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
