# CLI Reference

The FaceFusionCpp executable provides a powerful command-line interface (CLI) for both quick tasks and complex production pipelines.

**Usage Syntax**:
```powershell
FaceFusionCpp.exe [Global Options] [Quick Mode Options | Task Config Mode]
```

---

## 1. Global Options

These options control the base behavior of the application.

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `-v, --version` | None | Display application version. | `false` |
| `--app-config` | `<path>` | Path to the **global** application configuration file. | `config/app_config.yaml` |
| `--log-level` | `<level>` | Override log level (`trace`, `debug`, `info`, `warn`, `error`). | `info` |
| `--system-check`| None | Run environment self-check (CUDA, library versions). | `false` |
| `--json` | None | If set, `--system-check` results will be output in JSON format. | `false` |
| `--validate` | None | Parse and validate configuration file (Dry-Run) without executing. | `false` |

---

## 2. Quick Mode Options

Run tasks directly from the CLI. **Note**: Quick mode options are mutually exclusive with `-c/--task-config`.

| Option | Argument | Description | Example |
| :--- | :--- | :--- | :--- |
| `-s, --source` | `<path>` | Path(s) to source face image(s). Supports comma-separated list. | `-s a.jpg,b.jpg` |
| `-t, --target` | `<path>` | Path(s) to target media. Supports images, videos, or directories. | `-t movie.mp4` |
| `-o, --output` | `<path>` | Output path. Absolute paths are recommended. | `-o D:/output/` |
| `--processors` | `<list>` | Define pipeline steps (comma-separated). | `--processors face_swapper` |

> [!TIP]
> In Quick Mode, the app automatically loads `default_task_settings` from `app_config.yaml` as the foundation.

---

## 3. Task Configuration Mode

For complex workflows or batch processing, use YAML.

| Option | Argument | Description |
| :--- | :--- | :--- |
| `-c, --task-config` | `<path>` | Specify path to a task configuration file (YAML). |

---

## 4. Examples & Advanced Usage

### 4.1 Readiness Check (JSON Integration)
```powershell
FaceFusionCpp.exe --system-check --json
```
Output Example:
```json
{
  "checks": [
    {"name": "cuda_driver", "status": "ok", "value": "12.4"},
    {"name": "vram", "status": "warn", "value": "6.2GB", "message": "Recommended: 8GB+"}
  ],
  "summary": {"ok": 6, "warn": 1, "fail": 0}
}
```

### 4.2 Dry-Run Validation
Validate your YAML before submitting long-running tasks:
```powershell
FaceFusionCpp.exe -c my_complex_task.yaml --validate
```

### 4.3 Basic Swap + Enhance
```powershell
FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```
