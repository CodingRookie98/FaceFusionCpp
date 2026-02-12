# CLI Reference

The FaceFusionCpp executable provides a command-line interface (CLI) for both quick tasks and complex batch processing.

**Usage Syntax**:
```powershell
FaceFusionCpp.exe [options]
```

---

## 1. Global Options

These options apply to all running modes.

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `-v, --version` | None | Display application version and exit. | `false` |
| `--app-config` | `<file_path>` | Path to the global application configuration file. | `config/app_config.yaml` |
| `--log-level` | `<level>` | Override the logging level defined in config. <br>Values: `trace`, `debug`, `info`, `warn`, `error`. | `info` |
| `--system-check`| None | Run a system environment check (CUDA, libraries) and exit. | `false` |
| `--json` | None | Output system check results in JSON format. | `false` |
| `--validate` | None | Parse and validate the configuration file without running the task. | `false` |

---

## 2. Quick Mode Options

Use these options to run a task directly from the command line without creating a task config file.
> **Note**: These options cannot be used with `-c/--config`.

| Option | Argument | Description | Example |
| :--- | :--- | :--- | :--- |
| `-s, --source` | `<path>` | Path(s) to the source face image(s). Supports multiple sources. | `-s face1.jpg` |
| `-t, --target` | `<path>` | Path(s) to the target image or video. Supports multiple targets. | `-t video.mp4` |
| `-o, --output` | `<path>` | Output file path (single target) or directory (multiple targets). | `-o result.mp4` |
| `--processors` | `<list>` | Comma-separated list of processors to enable. | `--processors face_swapper,face_enhancer` |

**Available Processors**:
*   `face_swapper`: Replaces faces in target with source face.
*   `face_enhancer`: Enhances face details (upscaling/restoration).
*   `expression_restorer`: Restores original expression after swapping (Planned).
*   `frame_enhancer`: Enhances the entire frame (Super-Resolution).

---

## 3. Configuration Mode Options

For complex tasks or batch processing, use a YAML configuration file.

| Option | Argument | Description |
| :--- | :--- | :--- |
| `-c, --config` | `<file_path>` | Path to the task configuration file (YAML). |

---

## 4. Examples

### 4.1 Basic Face Swap
Swap faces from `source.jpg` to `target.jpg` and save to `output.jpg`.
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg
```

### 4.2 Face Swap + Enhancement
Perform swap and then enhance the face results.
```powershell
FaceFusionCpp.exe -s source.jpg -t target.jpg -o output.jpg --processors face_swapper,face_enhancer
```

### 4.3 Using a Config File
Run a task defined in `my_task.yaml`.
```powershell
FaceFusionCpp.exe -c my_task.yaml
```

### 4.4 Validate Configuration
Check if `my_task.yaml` is valid without running it.
```powershell
FaceFusionCpp.exe -c my_task.yaml --validate
```

### 4.5 System Check
Check if your environment (CUDA, libraries) is ready.
```powershell
FaceFusionCpp.exe --system-check
```

### 4.6 JSON System Check (for integrations)
```powershell
FaceFusionCpp.exe --system-check --json
```
