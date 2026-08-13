# 命令行参考 (CLI Reference)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-USER-ZH-CLI-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |


FaceFusionCpp 提供了一个强大的命令行界面 (CLI)，支持快速操作和复杂的批量处理任务。

**使用语法**:

**Linux (Bash)**:

```bash
./FaceFusionCpp [全局选项] [快捷模式选项 | 任务配置模式]
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe [全局选项] [快捷模式选项 | 任务配置模式]
```

---

## 1. 全局选项 (Global Options)

这些选项控制程序的基础行为。

| 选项 | 参数 | 说明 | 默认值 |
| :--- | :--- | :--- | :--- |
| `-v, --version` | 无 | 显示应用程序版本。 | `false` |
| `--app-config` | `<path>` | **全局**应用程序配置路径。 | `config/app_config.yaml` |
| `--log-level` | `<level>` | 覆盖配置的日志级别 (`trace`, `debug`, `info`, `warn`, `error`)。 | `info` |
| `--system-check`| 无 | 运行环境自检 (CUDA, 库版本等)。 | `false` |
| `--json` | 无 | 开启时，`--system-check` 的结果将以 JSON 格式输出。 | `false` |
| `--validate` | 无 | 解析并校验配置合法性 (Dry-Run)，不执行任务。支持 YAML 文件和快捷模式参数校验。 | `false` |

---

## 2. 快捷模式选项 (Quick Mode Options)

直接从命令行启动任务。**注意**: 快捷模式参数与 `-c/--task-config` 互斥。

| 选项 | 参数 | 说明 | 示例 |
| :--- | :--- | :--- | :--- |
| `-s, --source` | `<path>` | 源人脸图片路径。支持逗号分隔的多个路径。 | `-s a.jpg,b.jpg` |
| `-t, --target` | `<path>` | 目标媒体路径。支持图片、视频或目录。 | `-t video.mp4` |
| `-o, --output` | `<path>` | 输出路径。建议使用绝对路径。 | `-o D:/output/` |
| `--processors` | `<list>` | 定义流水线步骤 (逗号分隔)。 | `--processors face_swapper` |

> [!TIP]
> 快捷模式下，程序将自动加载 `app_config.yaml` 中的 `default_task_settings` 作为基础。

---

## 3. 处理器参数选项 (Processor Options)

处理器参数 CLI 标志从参数元数据动态生成，允许在快捷模式下细粒度控制每个处理器的行为。

**命名规则**: `--{processor-name}-{param-name}`（下划线转为连字符）。例如 `face_swapper` 的 `model` 参数对应 `--face-swapper-model`。

**互斥规则**: 所有处理器参数标志与 `--task-config` 互斥。使用处理器参数时不能同时指定 `-c/--task-config`。

| 处理器 | 参数 | CLI 标志 | 类型 | 可选值 | 默认值 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `face_swapper` | `model` | `--face-swapper-model` | String | `inswapper_128`, `inswapper_128_fp16` | `inswapper_128_fp16` |
| `face_swapper` | `face_selector_mode` | `--face-swapper-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `face_swapper` | `reference_face_path` | `--face-swapper-reference-face-path` | Path | - | - |
| `face_enhancer` | `model` | `--face-enhancer-model` | String | `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4` | `codeformer` |
| `face_enhancer` | `blend_factor` | `--face-enhancer-blend-factor` | Float | `[0.0, 1.0]` | `0.8` |
| `face_enhancer` | `face_selector_mode` | `--face-enhancer-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `face_enhancer` | `reference_face_path` | `--face-enhancer-reference-face-path` | Path | - | - |
| `expression_restorer` | `model` | `--expression-restorer-model` | String | `live_portrait` | `live_portrait` |
| `expression_restorer` | `restore_factor` | `--expression-restorer-restore-factor` | Float | `[0.0, 1.0]` | `0.8` |
| `expression_restorer` | `face_selector_mode` | `--expression-restorer-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `expression_restorer` | `reference_face_path` | `--expression-restorer-reference-face-path` | Path | - | - |
| `frame_enhancer` | `model` | `--frame-enhancer-model` | String | `real_esrgan_x2`, `real_esrgan_x2_fp16`, `real_esrgan_x4`, `real_esrgan_x4_fp16`, `real_esrgan_x8`, `real_esrgan_x8_fp16`, `real_hatgan_x4` | `real_esrgan_x4` |
| `frame_enhancer` | `enhance_factor` | `--frame-enhancer-enhance-factor` | Float | `[0.0, 1.0]` | `0.8` |

> [!NOTE]
> 处理器参数仅在快捷模式下生效。未指定的参数将使用上表中的默认值。

---

## 4. 任务配置模式

对于生产环境或复杂流水线，建议使用 YAML。

| 选项 | 参数 | 说明 |
| :--- | :--- | :--- |
| `-c, --task-config` | `<path>` | 指定任务配置文件路径。 |

---

## 5. 示例与高级用法

### 5.1 环境就绪检查 (JSON 集成)

**Linux (Bash)**:

```bash
./FaceFusionCpp --system-check --json
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe --system-check --json
```
输出示例：
```json
{
  "checks": [
    {"name": "cuda_driver", "status": "ok", "value": "12.4"},
    {"name": "vram", "status": "warn", "value": "6.2GB", "message": "Recommended: 8GB+"}
  ],
  "summary": {"ok": 6, "warn": 1, "fail": 0}
}
```

### 5.2 离线校验配置
支持校验 YAML 配置文件和快捷模式参数组合：

**校验 YAML 配置**:

**Linux (Bash)**:

```bash
./FaceFusionCpp -c my_complex_task.yaml --validate
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -c my_complex_task.yaml --validate
```

**校验快捷模式参数**:

**Linux (Bash)**:

```bash
./FaceFusionCpp -s face.jpg -t movie.mp4 -o out/ --processors face_swapper --validate
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ --processors face_swapper --validate
```

### 5.3 基础换脸 + 增强

**Linux (Bash)**:

```bash
./FaceFusionCpp -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```

### 5.4 快捷模式 + 处理器参数

使用处理器参数自定义每个处理器的行为：

**Linux (Bash)**:

```bash
./FaceFusionCpp -s face.jpg -t movie.mp4 -o out/ \
  --processors face_swapper,face_enhancer \
  --face-swapper-model inswapper_128 \
  --face-enhancer-model gfpgan_1.4 \
  --face-enhancer-blend-factor 0.9
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ `
  --processors face_swapper,face_enhancer `
  --face-swapper-model inswapper_128 `
  --face-enhancer-model gfpgan_1.4 `
  --face-enhancer-blend-factor 0.9
```

### 5.5 多处理器 + reference 模式

使用 reference 模式进行精准换脸：

**Linux (Bash)**:

```bash
./FaceFusionCpp -s face.jpg -t movie.mp4 -o out/ \
  --processors face_swapper,expression_restorer,frame_enhancer \
  --face-swapper-face-selector-mode reference \
  --face-swapper-reference-face-path ref.jpg \
  --frame-enhancer-model real_esrgan_x4_fp16
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ `
  --processors face_swapper,expression_restorer,frame_enhancer `
  --face-swapper-face-selector-mode reference `
  --face-swapper-reference-face-path ref.jpg `
  --frame-enhancer-model real_esrgan_x4_fp16
```
