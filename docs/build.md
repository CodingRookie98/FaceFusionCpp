# 构建脚本说明

项目使用 `build.ps1` 脚本进行配置和构建，支持 Debug 和 Release 两种配置。

## 基本用法

```powershell
# 配置并构建 Debug 版本（默认）
.\build.ps1

# 仅构建 Release 版本
.\build.ps1 -Configuration Release -Action build

# 仅配置 Debug 版本，使用 4 个并行任务
.\build.ps1 -Configuration Debug -Action configure -Jobs 4

# 配置并构建 Release 版本，使用 16 个并行任务
.\build.ps1 -Configuration Release -Action both -Jobs 16

# 运行测试
.\build.ps1 -Configuration Debug -Action test

# 安装项目
.\build.ps1 -Configuration Release -Action install

# 打包项目
.\build.ps1 -Configuration Release -Action package

# 启用代码覆盖率
.\build.ps1 -Configuration Debug -Action test -EnableCoverage

# 启用静态分析
.\build.ps1 -Configuration Debug -Action both -EnableStaticAnalysis
```

## 参数说明

- `-Configuration`: 构建配置类型，可选值为 `Debug` 或 `Release`，默认为 `Debug`
- `-Action`: 执行操作，可选值为：
  - `configure`（仅配置）
  - `build`（仅构建）
  - `test`（运行测试）
  - `install`（安装项目）
  - `package`（打包项目）
  - `both`（配置并构建，默认）
- `-Jobs`: 并行构建任务数，范围 1-32，默认为 8
- `-EnableCoverage`: 启用代码覆盖率（需要 Debug 配置）
- `-EnableStaticAnalysis`: 启用静态分析

## 完整工作流

### 1. 开发流程

```powershell
# 配置并构建 Debug 版本
.\build.ps1

# 运行测试
.\build.ps1 -Action test

# 运行测试并生成覆盖率报告
.\build.ps1 -Action test -EnableCoverage

# 启用静态分析进行构建
.\build.ps1 -Action both -EnableStaticAnalysis
```

### 2. 发布流程

```powershell
# 配置并构建 Release 版本
.\build.ps1 -Configuration Release

# 运行测试确保质量
.\build.ps1 -Configuration Release -Action test

# 安装到目标目录
.\build.ps1 -Configuration Release -Action install

# 打包为发布版本
.\build.ps1 -Configuration Release -Action package
```

### 3. 高级用法

```powershell
# 重新配置（清除缓存后重新配置）
Remove-Item -Recurse -Force build\msvc-x64-debug
.\build.ps1 -Action configure

# 仅构建特定目标
.\build.ps1 -Action build -Jobs 16

# 快速迭代开发（仅构建）
.\build.ps1 -Action build

# 完整的发布流程
.\build.ps1 -Configuration Release -Action both
.\build.ps1 -Configuration Release -Action test
.\build.ps1 -Configuration Release -Action install
.\build.ps1 -Configuration Release -Action package
```

## 输出目录

### Debug 配置
- **构建目录**: `build/msvc-x64-debug/`
- **可执行文件**: `build/msvc-x64-debug/runtime/msvc-x64-debug/FaceFusionCpp.exe`
- **安装目录**: `build/msvc-x64-debug/`

### Release 配置
- **构建目录**: `build/msvc-x64-release/`
- **可执行文件**: `build/msvc-x64-release/runtime/msvc-x64-release/FaceFusionCpp.exe`
- **安装目录**: `build/msvc-x64-release/`
- **打包文件**: `build/packages/Release/FaceFusionCpp-0.33.0-Windows-x86_64-Release.7z`

## 常见问题

### 1. CMake 未找到

**错误信息**: `CMake executable not found!`

**解决方案**:
- 安装 CMake: https://cmake.org/download/
- 确保 CMake 已添加到系统 PATH
- 或者安装到标准位置: `C:\Program Files\CMake\bin\`

### 2. Visual Studio 未找到

**错误信息**: `Visual Studio DevShell module not found!`

**解决方案**:
- 安装 Visual Studio 2022 Build Tools
- 确保安装了 C++ 构建工具
- 或者修改脚本中的路径指向正确的安装位置

### 3. 构建失败

**错误信息**: `Build failed with exit code: X`

**解决方案**:
- 检查编译错误信息
- 确保所有依赖库已正确安装
- 尝试清除构建目录后重新构建:
  ```powershell
  Remove-Item -Recurse -Force build\msvc-x64-debug
  .\build.ps1
  ```

### 4. 测试失败

**错误信息**: `Tests failed with exit code: X`

**解决方案**:
- 查看详细的测试输出
- 确保测试代码已正确实现
- 使用 `-EnableCoverage` 参数获取更多信息

### 5. 打包失败

**错误信息**: `Packaging failed with exit code: X`

**解决方案**:
- 确保已先完成构建和安装
- 检查 CPack 配置是否正确
- 查看打包日志了解详细错误

## 性能优化建议

### 1. 并行构建

根据 CPU 核心数调整并行任务数:
```powershell
# 8 核 CPU
.\build.ps1 -Jobs 8

# 16 核 CPU
.\build.ps1 -Jobs 16
```

### 2. 增量构建

在开发过程中使用 `build` 操作避免重新配置:
```powershell
# 首次配置
.\build.ps1 -Action configure

# 后续增量构建
.\build.ps1 -Action build
```

### 3. Release 优化

Release 版本会自动启用以下优化:
- 链接时优化 (LTO/IPO)
- 更高的优化级别
- 更小的二进制文件

## 集成到 CI/CD

### GitHub Actions 示例

```yaml
name: Build and Test

on: [push, pull_request]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup MSVC
        uses: microsoft/setup-msbuild@v1
        
      - name: Setup CMake
        uses: jwlawson/actions-setup-cmake@v2
        
      - name: Build Debug
        run: .\build.ps1 -Configuration Debug -Action both
        
      - name: Run Tests
        run: .\build.ps1 -Configuration Debug -Action test
        
      - name: Build Release
        run: .\build.ps1 -Configuration Release -Action both
        
      - name: Package Release
        run: .\build.ps1 -Configuration Release -Action package
```

## 相关文档

- [项目规范](../.trae/rules/project_rules.md)
- [CMake 文档](https://cmake.org/documentation/)
- [CPack 文档](https://cmake.org/cmake/help/latest/manual/cpack.1.html)
- [CTest 文档](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
