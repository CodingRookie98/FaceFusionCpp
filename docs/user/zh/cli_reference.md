# 命令行参考 (CLI Reference)

FaceFusionCpp 提供了一个强大的命令行界面 (CLI)，支持快速操作和复杂的批量处理任务。

**使用语法**:
```powershell
FaceFusionCpp.exe [全局选项] [快捷模式选项 | 任务配置模式]
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
| `--validate` | 无 | 解析并校验配置文件合法性 (Dry-Run)，不执行任务。 | `false` |

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

## 3. 任务配置模式

对于生产环境或复杂流水线，建议使用 YAML。

| 选项 | 参数 | 说明 |
| :--- | :--- | :--- |
| `-c, --task-config` | `<path>` | 指定任务配置文件路径。 |

---

## 4. 示例与高级用法

### 4.1 环境就绪检查 (JSON 集成)
```powershell
FaceFusionCpp.exe --system-check --json
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

### 4.2 离线校验配置
在提交长时任务前，先校验 YAML 格式：
```powershell
FaceFusionCpp.exe -c my_complex_task.yaml --validate
```

### 4.3 基础换脸 + 增强
```powershell
FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```
