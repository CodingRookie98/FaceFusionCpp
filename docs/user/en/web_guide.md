# Web UI Guide

> **Document Control**
> - **Document ID**: FFC-USER-EN-WEBUI-2026
> - **Version**: V1.0.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-17

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
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

- All tasks with ID, status, progress and media count;
- Auto-refreshes every 3 seconds; running tasks show a live progress bar;
- Cancel queued/running tasks.

### 2.2 Submit Task

- **Upload files**: upload buttons above source/target inputs support multi-file selection (image/video); uploaded paths are filled in automatically;
- **Source face paths**: one per line (or comma separated); server-side paths are accepted too;
- **Target media paths**: one per line, batch supported (multiple media in one task);
- **Processors**: check `face_swapper`, `face_enhancer`, `expression_restorer`, `frame_enhancer`;
- Output directory may be left empty (config defaults apply).

### 2.3 Task List (Queue & Priority)

- Queued tasks show **queue position** (#N, sorted by priority desc) and **priority** (P value, higher wins);
- Use `↑`/`↓` buttons to raise/lower priority of queued tasks; running tasks cannot be reprioritized.

### 2.3 Task Detail

- Live progress via WebSocket (frame count, FPS);
- Cancel task;
- On completion: **before/after comparison** (side by side) and downloadable result files.

## 3. API Summary

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Health check (with version) |
| `POST` | `/api/tasks` | Submit task (JSON: source_paths/target_paths/output_path/processors) |
| `GET` | `/api/tasks` | Task list |
| `GET` | `/api/tasks/{id}` | Task detail (incl. media/result URLs) |
| `POST` | `/api/tasks/{id}/cancel` | Cancel task |
| `POST` | `/api/tasks/{id}/priority` | Set priority (queued tasks only) |
| `POST` | `/api/upload` | Upload file (binary body + `X-File-Name` header) |
| `GET` | `/api/tasks/{id}/result` | Result files |
| `WS` | `/ws/tasks/{id}/progress` | Live progress push |
| `GET` | `/media/{task_id}/{kind}/{name}` | Media/result file access (whitelist) |

## 4. Development

- Frontend dev: `cd web && npm run dev` (Vite dev server :5173, HMR; API/WS proxied to `ffc --web` on :8000);
- Integration debug: `npm run build` then `./ffc --web --web-root web/dist`;
- Rebuild frontend assets: `python build.py --action web`.
