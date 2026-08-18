# Web UI Guide

> **Document Control**
> - **Document ID**: FFC-USER-EN-WEBUI-2026
> - **Version**: V1.2.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-18

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V1.2.0** | 2026-08-18 | AI Agent | 王辉 | Added dynamic processor parameters, video Range streaming playback, /api/processors, /api/tasks/{id}/progress, and one-click dev mode. |
| **V1.1.0** | 2026-08-17 | AI Agent | 王辉 | Added M4 video frame extraction (FrameExtractor), face detection & selection (FaceSelector), and reference face options. |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | Initial Web UI guide (M2: task lifecycle + preview). |

## 1. Starting the Web UI

Run from the executable directory:

**Linux (Bash)**:
```bash
./ffc --web
```

**Windows (PowerShell)**:
```powershell
.\ffc.exe --web
```

Default listen address `http://0.0.0.0:8000`; open `http://127.0.0.1:8000` in a browser.

Common options:

| Option | Description | Default |
| :--- | :--- | :--- |
| `--web-port` | Server port | `8000` |
| `--web-host` | Bind address | `0.0.0.0` |
| `--web-root` | Frontend assets root (point to `web/dist` when developing) | `assets/web` |

> [!NOTE]
> `--web` is mutually exclusive with quick mode (`-s/-t/-o`) and task config mode (`-c`).

## 2. Pages

### 2.1 Task List

- All tasks with ID, status, progress, media count, priority and queue position;
- Auto-refreshes every 3 seconds; running tasks show a live progress bar;
- Cancel queued/running tasks, or raise/lower priority of queued tasks (higher integer value = higher priority).

### 2.2 Submit Task

- **Upload files**: upload buttons above source/target inputs support multi-file selection (image/video); uploaded paths are filled in automatically;
- **📹 Video Frame Extractor (FrameExtractor)**: Expand tool panel, pick a local video, scrub and fine-tune playback time (±0.1s / ±1s), and capture the current frame as an input path;
- **👤 Face Detection & Selection (FaceSelector)**: Click "🔍 Detect Source Faces" to run `/api/faces`, displaying detected face bounding boxes that can be clicked to select specific faces;
- **Face Selector Mode**:
  - **Many**: Replaces all detected faces in the target media;
  - **One**: Replaces only the highest-confidence face;
  - **Reference**: Matches against a reference face image or selected face from the source;
- **Processors & Dynamic Parameters**:
  - Check `face_swapper`, `face_enhancer`, `expression_restorer`, `frame_enhancer`;
  - Selected processors dynamically render form controls based on metadata from `/api/processors` (e.g., model selection, `blend_factor`, `restore_factor`, `enhance_factor`);
- Output directory may be left empty (config defaults apply).

### 2.3 Task Detail

- Live progress via WebSocket (frame count, FPS);
- Cancel task;
- On completion: **before/after comparison** (side-by-side / slider for images, native `<video>` with Range streaming for videos) and downloadable result files.

## 3. API Summary

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Health check (with version and status) |
| `GET` | `/api/processors` | List of available processors and parameter schemas |
| `POST` | `/api/tasks` | Submit task (JSON: source_paths/target_paths/output_path/processors/processor_params) |
| `GET` | `/api/tasks` | Task list (with priority and queue position) |
| `GET` | `/api/tasks/{id}` | Task detail (incl. media/result URLs) |
| `GET` | `/api/tasks/{id}/progress` | Current task progress snapshot (WS disconnect compensation) |
| `POST` | `/api/tasks/{id}/cancel` | Cancel task |
| `POST` | `/api/tasks/{id}/priority` | Set priority (queued tasks only) |
| `POST` | `/api/upload` | Upload file (binary body + `X-File-Name` header) |
| `POST`/`GET` | `/api/faces` | Image face detection & annotation (bounding boxes, score, age/gender, landmarks) |
| `GET` | `/api/tasks/{id}/result` | Result files |
| `WS` | `/ws/tasks/{id}/progress` | Live progress push |
| `GET` | `/media/{task_id}/{kind}/{name}` | Media/result file access (supports HTTP Range / 206 video streaming) |

## 4. Development

- **One-click development**: `python build.py --action dev [--web-port 8000]` (launches backend and Vite HMR dev server);
- **Frontend dev**: `cd web && npm run dev` (Vite dev server :5173, HMR; API/WS proxied to `ffc --web` on :8000);
- **Integration debug**: `npm run build` then `./ffc --web --web-root web/dist`;
- **Rebuild frontend assets**: `python build.py --action web`.
