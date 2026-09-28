# 接口与构建规范 (Interfaces & Build Specification)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-DESIGN-IFACE-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 自 [design.md](./design.md) V3.3 拆分（文档治理 P3：单文档 ≤800 行红线），内容未变更。 |

> 所属总纲：[design.md](./design.md)

---

### 3.4 构建系统规范 (Build System Specification)
*   **单一事实来源 (Single Source of Truth)**: 必须使用 `CMakePresets.json` 定义所有构建预设 (Presets)，禁止在脚本中硬编码编译器标志。
*   **构建包装器**: `build.py` 仅作为跨平台包装器，负责调用 `cmake --preset` 并提供环境检查等辅助功能。
*   **开发预设**: 默认使用 `debug` 预设，确保启用所有调试符号与断言。
*   **发布预设**: 发布版本必须使用 `release` 预设，并启用 LTO (Link Time Optimization)。

### 3.5 命令行接口 (Command Line Interface)
本设计旨在平衡生产环境的配置管理需求与开发调试的便捷性。

#### 3.5.1 设计原则
*   **配置优先 (Configuration First)**: 生产环境应始终通过 `-c/--task-config` 加载完整 YAML，确保可复现性。
*   **参数覆盖 (CLI Override)**: 命令行显式参数优先级高于配置文件（例如在 Config 中定义了输出路径，但 CLI 又指定了 `-o`，则以 CLI 为准）。
*   **快捷模式 (Quick Run)**: 支持仅通过 CLI 参数 (`-s`, `-t`) 启动默认流水线，无需预先编写 YAML。
*   **元数据驱动 (Metadata-Driven)**: 处理器参数通过 ProcessorParamRegistry 元数据注册表自动生成 CLI 标志，确保参数定义的单一事实来源。

#### 3.5.2 命令结构
`ffc.exe [GLOBAL_OPTIONS] [TASK_OPTIONS] [PROCESSOR_FLAGS]`

#### 3.5.3 参数规格

| 类别     | 参数 (Short/Long) | 类型    | 描述                                                     |
| :------- | :---------------- | :------ | :------------------------------------------------------- |
| **全局** | `-h`, `--help`    | Flag    | 显示帮助与用法                                           |
|          | `-v`, `--version` | Flag    | 显示构建版本信息                                         |
|          | `-c`, `--task-config` | Path | **(核心)** 载入任务配置文件                              |
|          | `--log-level`     | String  | 覆盖日志级别                                             |
|          | `--validate`      | Flag    | 仅校验配置文件合法性，不执行任务 (Dry-Run)               |
|          | `--system-check`  | Flag    | 执行环境完整性自检（见下方输出规范）                     |
| **快捷** | `-s`, `--source`  | Path(s) | 源人脸图片路径 (支持多个，逗号分隔)                      |
|          | `-t`, `--target`  | Path(s) | 目标图片/视频路径 (支持多个)                             |
|          | `-o`, `--output`  | Path    | 输出目录或文件路径                                       |
|          | `--processors`    | String  | 启用的处理器 (逗号分隔，如 `face_swapper,face_enhancer`) |
| **处理器** (face_swapper) | `--face-swapper-model` | String | 模型选择 (enum: inswapper_128, inswapper_128_fp16) |
|          | `--face-swapper-face-selector-mode` | String | 人脸选择模式 (enum: reference, one, many) |
|          | `--face-swapper-reference-face-path` | Path | 参考人脸图片路径 (mode=reference 时必需) |
| **处理器** (face_enhancer) | `--face-enhancer-model` | String | 模型选择 (enum: codeformer, gfpgan_1.2, gfpgan_1.3, gfpgan_1.4) |
|          | `--face-enhancer-blend-factor` | Float | 混合因子 (0-1, 默认 0.8) |
|          | `--face-enhancer-face-selector-mode` | String | 人脸选择模式 (enum: reference, one, many) |
|          | `--face-enhancer-reference-face-path` | Path | 参考人脸图片路径 (mode=reference 时必需) |
| **处理器** (expression_restorer) | `--expression-restorer-model` | String | 模型选择 (enum: live_portrait) |
|          | `--expression-restorer-restore-factor` | Float | 还原因子 (0-1, 默认 0.8) |
|          | `--expression-restorer-face-selector-mode` | String | 人脸选择模式 (enum: reference, one, many) |
|          | `--expression-restorer-reference-face-path` | Path | 参考人脸图片路径 (mode=reference 时必需) |
| **处理器** (frame_enhancer) | `--frame-enhancer-model` | String | 模型选择 (enum: real_esrgan_x2, real_esrgan_x2_fp16, real_esrgan_x4, real_esrgan_x4_fp16, real_esrgan_x8, real_esrgan_x8_fp16, real_hatgan_x4) |
|          | `--frame-enhancer-enhance-factor` | Float | 增强因子 (0-1, 默认 0.8) |

> **注意**: 快捷模式参数与 `--task-config` 互斥。使用快捷参数时，系统将应用 `default_task_settings` 中的默认值。

> **注意**: 处理器参数标志**并非静态表格**——由 `ProcessorParamRegistry` 元数据注册表在启动时**动态生成**（`--{processor}-{param}` kebab-case 规则），确保参数定义与代码实现保持同步。下表仅为 4 个内置处理器当前注册参数的**示例快照**，实际可用标志以 `ffc --help` 输出为准。所有处理器参数均不包含 `--task-config` 标志。

#### 3.5.4 `--system-check` 输出规范
系统自检结果支持两种输出格式，便于人工查看与脚本集成：

**默认 (人类可读)**:
```text
[OK] CUDA Driver: 12.4
[OK] cuDNN: 8.9.7
[OK] TensorRT: 10.0.1
[WARN] Available VRAM: 6.2GB (Recommended: 8GB+)
[OK] Model Repository: 12 models found
[OK] FFmpeg Libraries: avcodec 60.3.100, avformat 60.3.100
[OK] ONNX Runtime: 1.17.0 (CUDA EP)
---
Result: 0 FAIL, 1 WARN
```

**JSON 格式** (添加 `--json` 标志):
```json
{
  "checks": [
    {"name": "cuda_driver", "status": "ok", "value": "12.4"},
    {"name": "vram", "status": "warn", "value": "6.2GB", "message": "Recommended: 8GB+"},
    {"name": "ffmpeg_libs", "status": "ok", "value": "avcodec 60.3.100, avformat 60.3.100"},
    {"name": "onnxruntime", "status": "ok", "value": "1.17.0", "provider": "CUDA"}
  ],
  "summary": {"ok": 6, "warn": 1, "fail": 0}
}
```

> **注意**: FFmpeg 使用动态库集成，而非命令行工具。详见 [附录 A.4 依赖项说明](#a4-依赖项说明-dependencies)。

---

### A.4 依赖项说明 (Dependencies)

本项目核心外部依赖的集成方式如下：

| 依赖项           | 集成方式        | 说明                                                                                                                |
| :--------------- | :-------------- | :------------------------------------------------------------------------------------------------------------------ |
| **FFmpeg**       | 动态库 (Shared) | 使用 `libavcodec`, `libavformat`, `libavutil`, `libswscale` 等库进行视频解码/编码。**不依赖 `ffmpeg` 命令行工具**。 |
| **ONNX Runtime** | 动态库 (Shared) | 提供跨后端 (CPU/CUDA/TensorRT) 的统一推理接口。                                                                     |
| **TensorRT**     | 动态库 (Shared) | 通过 ONNX Runtime TensorRT Execution Provider 加速推理。                                                            |
| **CUDA/cuDNN**   | 系统安装        | GPU 计算基础设施，需预装 CUDA Toolkit 及 cuDNN。                                                                    |
| **OpenCV**       | 静态库 (Static) | 图像处理基础库，编译时静态链接。                                                                                    |
| **spdlog**       | Header-Only     | 高性能日志库。                                                                                                      |
| **yaml-cpp**     | 静态库 (Static) | YAML 配置解析。                                                                                                     |

> [!IMPORTANT]
> **FFmpeg 集成说明**：本项目通过 C++ 直接调用 FFmpeg 动态库 API（如 `avcodec_open2`, `av_read_frame` 等），而非通过 `subprocess` 调用命令行工具。这种方式：
> - ✅ 无需将 `ffmpeg.exe` 放入 PATH
> - ✅ 减少进程间通信开销
> - ✅ 更精细的错误处理与资源管理
> - ⚠️ 需确保运行时能找到 FFmpeg `.dll`/`.so` 文件（建议放置于 `bin/` 目录或系统库路径）
