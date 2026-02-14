# 命令行参考 (CLI Reference)

FaceFusionCpp 提供了一个强大的命令行界面 (CLI)，支持快速操作和复杂的批量处理任务。

**使用语法**:
```powershell
FaceFusionCpp.exe [选项]
```

---

## 1. 全局选项 (Global Options)

这些选项适用于所有运行模式。

| 选项 | 参数 | 说明 | 默认值 |
| :--- | :--- | :--- | :--- |
| `-v, --version` | 无 | 显示应用程序版本并退出。 | `false` |
| `--app-config` | `<file_path>` | 全局应用程序配置文件的路径。 | `config/app_config.yaml` |
| `--log-level` | `<level>` | 覆盖配置中定义的日志级别。<br>可选值: `trace`, `debug`, `info`, `warn`, `error`。 | `info` |
| `--system-check`| 无 | 运行系统环境检查 (CUDA, 库版本等) 并退出。 | `false` |
| `--json` | 无 | 以 JSON 格式输出系统检查结果。 | `false` |
| `--validate` | 无 | 解析并验证配置文件，但不运行任务。 | `false` |

---

## 2. 快捷模式选项 (Quick Mode Options)

使用这些选项直接从命令行运行任务，无需创建任务配置文件。
> **注意**: 这些选项不能与 `-c/--task-config` 同时使用。

| 选项 | 参数 | 说明 | 示例 |
| :--- | :--- | :--- | :--- |
| `-s, --source` | `<path>` | 源人脸图片路径。支持多个源。 | `-s face1.jpg` |
| `-t, --target` | `<path>` | 目标图片或视频路径。支持多个目标。 | `-t video.mp4` |
| `-o, --output` | `<path>` | 输出文件路径 (单个目标) 或目录 (多个目标)。 | `-o result.mp4` |
| `--processors` | `<list>` | 要启用的处理器列表 (逗号分隔)。 | `--processors face_swapper,face_enhancer` |

**可用处理器**:
*   `face_swapper`: 将目标中的人脸替换为源人脸。
*   `face_enhancer`: 增强人脸细节 (超分/修复)。
*   `expression_restorer`: 在换脸后恢复原始表情 (计划中)。
*   `frame_enhancer`: 增强整个画面 (超分)。

---

## 3. 配置模式选项 (Configuration Mode Options)

对于复杂任务或批量处理，请使用 YAML 配置文件。

| 选项 | 参数 | 说明 |
| :--- | :--- | :--- |
| `-c, --task-config` | `<file_path>` | 任务配置文件 (YAML) 的路径。 |

---

## 4. 示例 (Examples)

### 4.1 基础换脸
将 `source.jpg` 中的人脸换到 `target.jpg` 中，保存为 `output.jpg`。
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg
```

### 4.2 换脸 + 增强
执行换脸并对结果进行人脸增强。
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg --processors face_swapper,face_enhancer
```

### 4.3 使用配置文件
运行定义在 `my_task.yaml` 中的任务。
```powershell
FaceFusionCpp.exe -c my_task.yaml
```

### 4.4 验证配置
检查 `my_task.yaml` 是否有效，但不执行。
```powershell
FaceFusionCpp.exe -c my_task.yaml --validate
```

### 4.5 系统检查
检查您的环境 (CUDA, 库) 是否准备就绪。
```powershell
FaceFusionCpp.exe --system-check
```

### 4.6 JSON 系统检查 (用于集成)
```powershell
FaceFusionCpp.exe --system-check --json
```
