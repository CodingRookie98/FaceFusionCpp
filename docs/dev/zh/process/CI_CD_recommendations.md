# CI/CD 开发与运行建议

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PROC-CICD-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |

为了确保 FaceFusionCpp 在持续集成（CI）和持续交付（CD）环境中的稳定性，建议遵循以下准则。

---

## 1. 资源路径配置

CI 环境中的文件结构可能与本地开发环境不同。

### 指定 Assets 路径

如果测试框架无法自动定位到 `assets` 目录，可以显式指定：

```bash
export FACEFUSION_ASSETS_PATH="/absolute/path/to/project/assets"
```

---

## 2. 构建配置

### 使用 Release 配置进行集成测试

虽然 Debug 配置有助于排查逻辑问题，但在 CI 中，**MUST** 至少运行一次 Release 配置的测试，以验证编译器优化是否引入了并发或时序问题。

```bash
python build.py --config Release --action test
```

### 构建预设一览

| 预设名称 | 平台 | 构建类型 | 用途 |
| :--- | :--- | :--- | :--- |
| `linux-debug` | Linux x64 | Debug | 日常开发 |
| `linux-release` | Linux x64 | Release | CI 验证 / 发布 |
| `msvc-x64-debug` | Windows x64 | Debug | Windows 开发 |
| `msvc-x64-release` | Windows x64 | Release | Windows CI / 发布 |

---

## 3. GitHub Actions CI 配置

### 3.1 环境变量清单

| 变量名 | 必需 | 说明 | 示例 |
| :--- | :--- | :--- | :--- |
| `FACEFUSION_ASSETS_PATH` | 否 | 模型资源目录路径 | `/home/runner/work/.../assets` |
| `VCPKG_ROOT` | 是 | vcpkg 安装路径 | 由 CI 脚本自动设置 |
| `CUDA_VERSION` | 否 | CUDA 版本（GPU 测试时） | `12.4` |

### 3.2 推荐 CI Pipeline

```yaml
# .github/workflows/ci.yml
name: CI

on:
  push:
    branches: [dev, master]
  pull_request:
    branches: [dev]

env:
  VCPKG_DEFAULT_TRIPLET: x64-linux

jobs:
  # ──────────────────────────────────────────
  # Job 1: 代码质量检查（快速，< 2分钟）
  # ──────────────────────────────────────────
  lint:
    name: Code Quality
    runs-on: ubuntu-24.04
    steps:
      - uses: actions/checkout@v4

      - name: Install clang-tidy
        run: sudo apt-get install -y clang-tidy

      - name: Format check
        run: python scripts/format_code.py --check

      - name: Static analysis
        run: python scripts/run_clang_tidy.py

  # ──────────────────────────────────────────
  # Job 2: 构建 + 单元测试（矩阵）
  # ──────────────────────────────────────────
  build-and-test:
    name: ${{ matrix.os }} / ${{ matrix.config }}
    needs: lint
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: ubuntu-24.04
            preset-prefix: linux
            compiler: clang
          - os: windows-2022
            preset-prefix: msvc-x64
            compiler: msvc
        config: [debug, release]

    runs-on: ${{ matrix.os }}

    steps:
      - uses: actions/checkout@v4

      # vcpkg 缓存
      - name: Cache vcpkg
        uses: actions/cache@v4
        with:
          path: |
            build/vcpkg_installed
            ${{ env.VCPKG_ROOT }}
          key: vcpkg-${{ matrix.os }}-${{ hashFiles('vcpkg.json') }}
          restore-keys: vcpkg-${{ matrix.os }}-

      # Linux 依赖
      - name: Install Linux dependencies
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y ninja-build clang lld

      # Windows 依赖
      - name: Install Windows dependencies
        if: runner.os == 'Windows'
        uses: ilammy/msvc-dev-cmd@v1

      # 构建
      - name: Configure
        run: python build.py --preset ${{ matrix.preset-prefix }}-${{ matrix.config }} --action configure

      - name: Build
        run: python build.py --preset ${{ matrix.preset-prefix }}-${{ matrix.config }} --action build

      # 单元测试
      - name: Unit Tests
        run: python build.py --preset ${{ matrix.preset-prefix }}-${{ matrix.config }} --action test --test-label unit

      # 集成测试（仅 Release）
      - name: Integration Tests
        if: matrix.config == 'release'
        run: python build.py --preset ${{ matrix.preset-prefix }}-${{ matrix.config }} --action test --test-label integration

      # 上传测试结果
      - name: Upload Test Results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: test-results-${{ matrix.os }}-${{ matrix.config }}
          path: build/**/test-results/
```

### 3.3 缓存策略

| 缓存对象 | 缓存键 | 预期收益 |
| :--- | :--- | :--- |
| vcpkg installed | `hashFiles('vcpkg.json')` | 减少 5-15 分钟依赖安装时间 |
| build/vcpkg_installed | 同上 | 避免重复编译依赖 |
| TensorRT engines | 模型哈希 | 避免重复引擎构建（如适用） |

---

## 4. 基础 CD：打包发布

### 4.1 触发条件

```yaml
# .github/workflows/release.yml
on:
  push:
    tags:
      - 'v*'
```

### 4.2 发布流程

```mermaid
graph LR
    A[Tag 推送] --> B[CI 全量测试]
    B --> C[构建 Release 包]
    C --> D[上传 GitHub Release]
    D --> E[通知]
```

### 4.3 打包脚本

```bash
# 构建发布包
python build.py --preset linux-release --action package

# 产物目录结构
# build/install/linux-x64-release/
# ├── bin/
# │   └── ffc
# ├── config/
# │   ├── app_config.yaml
# │   └── task_config.yaml
# └── assets/
#     └── models/ (可选，体积大)
```

### 4.4 GitHub Release 配置

```yaml
      - name: Create Release
        uses: softprops/action-gh-release@v2
        with:
          files: |
            build/install/**/bin/ffc
            build/install/**/bin/*.dll
          draft: false
          prerelease: ${{ contains(github.ref, 'rc') }}
```

---

## 5. 故障排查

### 5.1 常见问题决策树

```
CI 失败
├── 编译失败
│   ├── vcpkg 安装超时 → 检查缓存是否命中，增加 timeout
│   ├── 缺少系统依赖 → 检查 apt-get 步骤
│   └── 模块编译错误 → 本地复现，检查 CMakeLists.txt
├── 测试失败
│   ├── 单元测试失败 → 检查代码变更，本地复现
│   ├── 集成测试失败 → 检查 GPU 资源是否可用
│   └── 超时 → 检查是否有死锁或无限循环
└── 环境问题
    ├── CUDA 不可用 → 检查 runner 是否有 GPU
    └── 内存不足 → 减少并行度或使用更大 runner
```

### 5.2 调试命令

```bash
# 本地复现 CI 环境
docker run -it ubuntu:24.04 bash

# 检查 vcpkg 状态
vcpkg list
vcpkg x-package-info <package>

# 检查构建产物
ls -la build/bin/linux-x64-debug/

# 运行单个测试
cd build/bin/linux-x64-debug && ./ffc_test --gtest_filter="*specific_test*"
```

### 5.3 失败处理策略

| 失败类型 | 处理策略 | 优先级 |
| :--- | :--- | :--- |
| lint 失败 | 阻断，不允许合并 | **MUST** |
| 单元测试失败 | 阻断，不允许合并 | **MUST** |
| 集成测试失败 | 阻断，不允许合并 | **MUST** |
| Release 构建失败 | 阻断，不允许打 Tag | **MUST** |
| 缓存未命中 | 允许，但记录警告 | **SHOULD** |
| 超时（> 30min） | 阻断，需优化或增加 timeout | **SHOULD** |

---

## 6. 本地验证（提交前）

在推送前，**SHOULD** 在本地运行以下命令验证：

```bash
# 1. 格式检查
python scripts/format_code.py

# 2. 静态分析（Linux only）
python scripts/run_clang_tidy.py

# 3. 构建
python build.py --action build

# 4. 单元测试
python build.py --action test --test-label unit

# 5. 集成测试
python build.py --action test --test-label integration

# 6. 预提交检查（综合）
python scripts/pre_commit_check.py
```

---

## 附录：相关文档

| 文档 | 路径 | 说明 |
| :--- | :--- | :--- |
| 构建指南 | `docs/dev/zh/guides/setup.md` | 环境搭建 |
| 工作流程 | `docs/dev/zh/process/workflow.md` | 开发流程 |
| 质量标准 | `docs/dev/zh/process/quality.md` | 代码质量 |
