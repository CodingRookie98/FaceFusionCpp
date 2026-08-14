# CLI Reference

> **Document Control**
> - **Document ID**: FFC-USER-EN-CLI-2026
> - **Version**: V1.1.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-14

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-14 | AI Agent | 王辉 | Added Processor Options section (sync with zh); fixed default values to match app_config.yaml (face_enhancer/gfpgan_1.4, frame_enhancer/real_esrgan_x2_fp16); noted defaults source. |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | Initialized document control info per documentation governance. |


The FaceFusionCpp executable provides a powerful command-line interface (CLI) for both quick tasks and complex production pipelines.

**Usage Syntax**:

**Linux (Bash)**:

```bash
./FaceFusionCpp [Global Options] [Quick Mode Options | Task Config Mode]
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe [Global Options] [Quick Mode Options | Task Config Mode]
```

---

## 1. Global Options

These options control the base behavior of the application.

| Option | Argument | Description | Default |
| :--- | :--- | :--- | :--- |
| `-v, --version` | None | Display application version. | `false` |
| `--app-config` | `<path>` | Path to the **global** application configuration file. | `config/app_config.yaml` |
| `--log-level` | `<level>` | Override log level (`trace`, `debug`, `info`, `warn`, `error`). | `info` |
| `--system-check` | None | Run environment self-check (CUDA, library versions). | `false` |
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

## 3. Processor Options

Processor parameter CLI flags are generated dynamically from parameter metadata, allowing fine-grained control of each processor in Quick Mode.

**Naming Rule**: `--{processor-name}-{param-name}` (underscores become hyphens). For example, the `model` parameter of `face_swapper` maps to `--face-swapper-model`.

**Exclusivity Rule**: All processor parameter flags are mutually exclusive with `--task-config`.

| Processor | Parameter | CLI Flag | Type | Allowed Values | Default |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `face_swapper` | `model` | `--face-swapper-model` | String | `inswapper_128`, `inswapper_128_fp16` | `inswapper_128_fp16` |
| `face_swapper` | `face_selector_mode` | `--face-swapper-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `face_swapper` | `reference_face_path` | `--face-swapper-reference-face-path` | Path | - | - |
| `face_enhancer` | `model` | `--face-enhancer-model` | String | `codeformer`, `gfpgan_1.2`, `gfpgan_1.3`, `gfpgan_1.4` | `gfpgan_1.4` |
| `face_enhancer` | `blend_factor` | `--face-enhancer-blend-factor` | Float | `[0.0, 1.0]` | `0.8` |
| `face_enhancer` | `face_selector_mode` | `--face-enhancer-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `face_enhancer` | `reference_face_path` | `--face-enhancer-reference-face-path` | Path | - | - |
| `expression_restorer` | `model` | `--expression-restorer-model` | String | `live_portrait` | `live_portrait` |
| `expression_restorer` | `restore_factor` | `--expression-restorer-restore-factor` | Float | `[0.0, 1.0]` | `0.8` |
| `expression_restorer` | `face_selector_mode` | `--expression-restorer-face-selector-mode` | String | `reference`, `one`, `many` | `many` |
| `expression_restorer` | `reference_face_path` | `--expression-restorer-reference-face-path` | Path | - | - |
| `frame_enhancer` | `model` | `--frame-enhancer-model` | String | `real_esrgan_x2`, `real_esrgan_x2_fp16`, `real_esrgan_x4`, `real_esrgan_x4_fp16`, `real_esrgan_x8`, `real_esrgan_x8_fp16`, `real_hatgan_x4` | `real_esrgan_x2_fp16` |
| `frame_enhancer` | `enhance_factor` | `--frame-enhancer-enhance-factor` | Float | `[0.0, 1.0]` | `0.8` |

> [!NOTE]
> Processor parameters take effect only in Quick Mode. Unspecified parameters use the defaults above.
> Defaults come from the `default_models` section of `app_config.yaml` and can be adjusted there (no CLI change required).

---

## 4. Task Configuration Mode

For complex workflows or batch processing, use YAML.

| Option | Argument | Description |
| :--- | :--- | :--- |
| `-c, --task-config` | `<path>` | Specify path to a task configuration file (YAML). |

---

## 5. Examples & Advanced Usage

### 4.1 Readiness Check (JSON Integration)

**Linux (Bash)**:

```bash
./FaceFusionCpp --system-check --json
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe --system-check --json
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

**Linux (Bash)**:

```bash
./FaceFusionCpp -c my_complex_task.yaml --validate
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -c my_complex_task.yaml --validate
```

### 4.3 Basic Swap + Enhance

**Linux (Bash)**:

```bash
./FaceFusionCpp -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```

**Windows (PowerShell)**:

```powershell
.\FaceFusionCpp.exe -s face.jpg -t movie.mp4 -o out/ --processors face_swapper,face_enhancer
```
