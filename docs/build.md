# 使用 `build.py` 脚本配置和构建

本项目提供了 `build.py` 脚本，旨在简化配置、构建、测试和打包的流程。该脚本会自动检测系统环境，并支持 Windows、Linux 和 macOS（实验性）。

## 1. 快速开始

在终端中执行以下命令即可开始构建：

```bash
# 默认行为：配置并构建 Debug 版本
python build.py
```

## 2. 常用命令

```bash
# 仅构建 Release 版本
python build.py --config Release --action build

# 运行测试
python build.py --config Debug --action test

# 安装项目
python build.py --config Release --action install

# 打包项目
python build.py --config Release --action package

# 仅构建主程序
python build.py --target FaceFusionCpp

# 清理构建目录并重新构建
python build.py --clean --action both
```

## 3. 参数说明

脚本支持以下参数，用于定制构建行为：

| 参数 | 说明 | 可选值 | 默认值 |
| :--- | :--- | :--- | :--- |
| `--config` | 构建配置类型 | `Debug`, `Release` | `Debug` |
| `--action` | 执行的操作 | `configure` (仅配置)<br>`build` (仅构建)<br>`test` (构建并运行测试)<br>`install` (安装)<br>`package` (打包)<br>`both` (配置+构建) | `both` |
| `--target` | 构建目标 | `all` 或具体目标名 | `all` |
| `--preset` | 手动指定 CMake Preset | CMakePresets.json 中定义的名称 | 自动检测 |
| `--clean` | 清理构建目录 | `[flag]` | `False` |

> **注意**: 脚本会自动利用系统所有可用核心进行并行构建，无需手动指定 `-j` 参数。

## 4. 完整工作流示例

### 开发流程
```bash
# 1. 首次构建 Debug 版本
python build.py

# 2. 运行测试并查看结果
python build.py --action test
```

### 发布流程
```bash
# 1. 一键构建并测试 Release 版本
python build.py --config Release --action both
python build.py --config Release --action test

# 2. 生成安装包
python build.py --config Release --action package
```

## 5. 代码风格与静态检查

本项目使用 Clang 工具链进行代码格式化和静态分析。

### 代码格式化 (Clang-Format)

使用 `scripts/format_code.py` 脚本自动格式化代码。该脚本会根据 `.clang-format` 配置文件递归扫描 `src`, `facefusionCpp`, `tests` 目录下的 C++ 源文件。

```bash
python scripts/format_code.py
```

### 静态分析 (Clang-Tidy)

使用 `scripts/run_clang_tidy.py` 脚本运行静态分析。该脚本依赖于 CMake 配置生成的 `compile_commands.json`。

```bash
# 1. 确保已配置项目 (生成 compile_commands.json)
python build.py --action configure

# 2. 运行 Clang-Tidy 检查
python scripts/run_clang_tidy.py
```

> **MSVC 用户注意**: 当检测到使用 MSVC 编译器时，静态分析脚本会自动跳过 `.ixx` 和 `.cppm` (C++ 模块) 文件，因为 Clang-Tidy 对 MSVC 模块的支持尚不完善。

### Git Pre-commit Hook

本项目提供了 pre-commit hook，可以在提交前自动格式化代码并运行静态检查。

```bash
# 安装 hook
python scripts/install_hooks.py
```

## 6. 输出目录

构建产物将位于 `build` 目录下：

- **Windows Debug**: `build/msvc-x64-debug/`
- **Linux Debug**: `build/linux-debug/`
- **Release 构建**: `build/*-release/`
- **安装目录**: 对应构建目录下的 install 文件夹
- **打包文件**: 对应构建目录下或 `build/packages/`

---

# 使用 CMake 直接构建

如果您更喜欢使用原生的 CMake 命令行工具，或者使用 IDE（如 VS Code, Visual Studio, CLion）内置的 CMake 支持，可以直接使用 CMake Presets。

## 1. 环境准备

- **CMake**: 3.25 或更高版本。
- **编译器**: 
  - Windows: Visual Studio 2022 (MSVC)。
  - Linux: GCC 或 Clang。
- **构建系统**: Ninja (推荐)。
- **Windows 环境设置**:
  - 如果使用 `python build.py`，脚本会自动加载 MSVC 环境。
  - 如果手动运行 `cmake`，请在 `Visual Studio 2022 Developer PowerShell` 中运行。

## 2. 常用操作

项目利用 `CMakePresets.json` 预定义了常用的构建配置。

### 配置 (Configure)

```bash
# 查看所有可用预设
cmake --list-presets

# 配置 Debug 版本 (Windows)
cmake --preset msvc-x64-debug

# 配置 Debug 版本 (Linux)
cmake --preset linux-debug
```

### 构建 (Build)

```bash
# 构建 Debug 版本
cmake --build --preset msvc-x64-debug
```

### 测试 (Test)

```bash
# 运行 Debug 测试
ctest --preset msvc-x64-debug
```

## 3. 常见问题 (FAQ)

### Q: 提示 `clang-format` 或 `clang-tidy` not found?
**A**: 请安装 LLVM/Clang 工具链并确保其 bin 目录在系统 PATH 中。

### Q: Windows 上手动运行 CMake 找不到编译器?
**A**: 确保您在 Visual Studio Developer Command Prompt/PowerShell 中运行，或者使用 `python build.py` (它会自动处理环境)。
