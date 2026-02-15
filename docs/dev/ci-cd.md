# CI/CD 流程文档

本项目使用 GitHub Actions 进行持续集成和持续交付。

## 工作流说明

### 1. CI 工作流 (`ci.yml`)

- **触发条件**:
  - 推送至 `master`, `main`, `dev`, `*dev` 分支。
  - 针对 `master`, `main`, `dev` 分支的 Pull Request。
- **主要步骤**:
  - 代码格式检查 (`python scripts/format_code.py --check`)。
  - 环境初始化 (CUDA Toolkit, vcpkg)。
  - 项目构建 (Linux & Windows)。
  - 单元测试运行 (`python build.py --action test --test-label unit`)。

### 2. Release 工作流 (`release.yml`)

- **触发条件**: 推送以 `v` 开头的标签 (例如 `v1.0.0`)。
- **主要步骤**:
  - 在 Linux 和 Windows 上构建 Release 版本。
  - 使用 `cpack` 进行打包。
  - 自动创建 GitHub Release 并上传二进制包。

## 依赖管理与缓存

- **vcpkg**: 使用 GitHub Packages (NuGet) 作为二进制缓存后端，显著缩短构建时间。
- **CUDA**: 在 CI 环境中安装 CUDA Toolkit 以验证编译，但跳过 GPU 执行测试。

## 手动模型下载

由于模型文件较大，CI/CD 流程中**不包含**模型文件。用户需在运行程序时让其自动下载，或根据 `docs/user/en/getting_started.md` 手动下载并放置在指定目录。
