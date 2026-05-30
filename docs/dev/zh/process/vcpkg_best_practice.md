在现代 C++ 工程化实践中，`vcpkg.json` 是 **清单模式 (Manifest Mode)** 的核心。一个“标准”且具备工程化水准的 `vcpkg.json` 不仅仅是列出依赖，更要解决**版本锁定、模块化功能开启**以及**平台兼容性**等问题。

以下是一个标准工程化 `vcpkg.json` 的模版及其核心组成部分的详细拆解。

---

## 1. 标准工程化模版

```json
{
  "name": "my-awesome-project",
  "version": "1.2.3",
  "description": "这是一个标准的 C++ 工程化项目示例",
  "homepage": "https://github.com/user/project",
  "license": "MIT",
  "builtin-baseline": "364924630046098045610667e415a7538ec6da0e",
  "dependencies": [
    "fmt",
    "nlohmann-json",
    {
      "name": "opencv4",
      "features": [ "ffmpeg", "png", "jpeg" ]
    }
  ],
  "features": {
    "tests": {
      "description": "单元测试模块",
      "dependencies": [ "gtest" ]
    },
    "gui": {
      "description": "图形界面扩展",
      "dependencies": [ "qt5" ]
    }
  },
  "overrides": [
    {
      "name": "fmt",
      "version": "10.1.1"
    }
  ]
}

```

---

## 2. 核心字段深度解析

### A. 身份识别 (Identity)

* **`name`**: 必须是小写字母、数字或连字符（kebab-case）。
* **`version`**: 建议使用 `version`（简单字符串）或 `version-semver`（语义化版本）。

### B. 版本控制的关键：`builtin-baseline`

这是工程化的**重中之重**。

* **作用**：它是一个 vcpkg 官方仓库的 Git Commit ID。它告诉 vcpkg：“请将所有库的版本锁定在这个时间点的状态”。
* **意义**：保证了团队开发时，所有人拉取的依赖版本完全一致，避免了“我这里能编译，你那里报错”的经典坑。

### C. 依赖管理 (Dependencies)

* **基础依赖**：简单的字符串列表。
* **定制依赖**：使用对象形式，可以指定开启该库的哪些 `features`（例如 OpenCV 是否需要 ffmpeg 支持）。

### D. 模块化功能 (Features)

这是区分“玩具项目”和“专业项目”的标志。

* 你可以定义可选功能（如 `tests` 或 `gui`）。
* **使用场景**：当你在 CMake 中配置 `-DVCPKG_MANIFEST_FEATURES=tests` 时，vcpkg 才会去下载 `gtest`。这能显著减少持续集成（CI）时的构建时间。

### E. 版本覆盖 (Overrides)

* 如果 baseline 里的版本太旧，或者你需要某个特定版本的库，可以使用 `overrides` 强制指定版本。

---

## 3. 字段功能对比表

| 字段 | 强制性 | 目的 | 工程化建议 |
| --- | --- | --- | --- |
| **`name`** | 是 | 项目标识 | 统一使用小写和连字符 |
| **`builtin-baseline`** | 强烈建议 | **版本锁定** | 必须提供，以确保构建可重复性 |
| **`dependencies`** | 是 | 核心依赖列表 | 尽量保持精简 |
| **`features`** | 否 | 模块化依赖 | 将测试、文档、GUI 依赖分离 |
| **`supports`** | 否 | 平台限制 | 如果库不支持 Windows，需在此声明避免误用 |

---

## 4. 工程化最佳实践建议

1. **配合 `vcpkg-configuration.json` 使用**：
   不仅用于配置私有 Registry，更重要的是通过 **Overlay Ports** 处理定制需求（如修改 ffmpeg 编译参数），避免直接修改 vcpkg 源码。

2. **启用二进制缓存 (Binary Caching)**:
   这是 C++ 工程化的核心。配置 `VCPKG_BINARY_SOURCES`（如本地文件共享或 Artifactory），实现“一人编译，全员（包括 CI）复用”，显著降低构建时间。

3. **版本控制策略**：
   - **不要手动修改** `builtin-baseline`：建议使用 `vcpkg x-update-baseline` 命令更新。
   - **慎用 Overrides**：`overrides` 会打破基线承诺。除非有版本冲突或紧急 Bug 修复，否则优先通过更新 `builtin-baseline` 来整体升级。
   - **版本约束**：优先使用 `version>=` 确保最低版本。

4. **保持整洁**：
   - 提交代码前运行 `vcpkg format-manifest`，自动排序依赖并规范化 JSON 格式，减少 Merge Conflict。

> **避坑指南**：如果你在 VS/VS Code 中发现 vcpkg 没生效，请检查你的 CMake 设置是否开启了 `VCPKG_MANIFEST_MODE`（通常只要目录下有 `vcpkg.json` 且使用了 vcpkg 工具链文件，它会自动开启）。

---

## 5. FaceFusionCpp 项目实际配置

### 5.1 当前 vcpkg.json 配置

项目使用以下依赖配置（截至 v0.34.1）：

```json
{
  "name": "facefusioncpp",
  "version": "0.34.1",
  "builtin-baseline": "6d7bf7ef2193e2d1c5798a5ff8811d533104c861",
  "dependencies": [
    "cli11",           // CLI 参数解析
    "curl",            // HTTP 客户端（模型下载）
    "dp-thread-pool",  // 线程池
    { "name": "ffmpeg", "features": ["avcodec", "avdevice", "avfilter", "avformat", "gpl", "nvcodec", "swresample", "swscale", "x264", "x265"] },
    "flatbuffers",     // 序列化（Batch 模式）
    "indicators",      // 进度条
    "nlohmann-json",   // JSON 解析
    "onnx",            // ONNX 模型支持
    "opencv",          // 图像处理
    "openssl",         // HTTPS 支持
    "spdlog",          // 日志
    "yaml-cpp"         // YAML 配置解析
  ],
  "features": {
    "test": { "dependencies": ["gtest"] }
  }
}
```

### 5.2 已知兼容性问题与 Patch

| 库 | 问题 | 解决方案 | 自动化 |
| :--- | :--- | :--- | :--- |
| **OpenCV** | `types.hpp` 的 C++20 模板歧义 | `scripts/patch_opencv.py` | CMakeLists.txt 自动调用 |

### 5.3 vcpkg 更新流程

```bash
# 1. 更新 baseline 到最新
vcpkg x-update-baseline

# 2. 格式化 manifest
vcpkg format-manifest

# 3. 重新安装依赖
vcpkg install

# 4. 运行 OpenCV patch（如需要）
python scripts/patch_opencv.py

# 5. 验证构建
python build.py --action build
```

### 5.4 添加新依赖的检查清单

添加新依赖时，**MUST** 确认以下事项：

| # | 检查项 | 验证方式 |
| :--- | :--- | :--- |
| 1 | 依赖是否支持 Clang 21 | 查阅依赖文档或测试编译 |
| 2 | 依赖是否支持 C++20 | 查阅依赖文档 |
| 3 | 依赖是否有已知的 C++20 兼容性问题 | 搜索 GitHub Issues |
| 4 | 依赖是否需要 feature flags | 查阅 vcpkg port 文档 |
| 5 | 依赖是否需要 patch | 测试编译后确认 |
| 6 | 更新 vcpkg.json 后运行 `vcpkg format-manifest` | 自动格式化 |