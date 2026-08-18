# Web User Interface (Web UI) Design Specification

> **Document Control Information**
> - **Document ID**: FFC-DEV-EN-ARCH-WEBUI-2026
> - **Current Version**: V0.3.2
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: Hui Wang
> - **Reviewer**: Hui Wang
> - **Last Updated**: 2026-08-18

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V0.3.2** | 2026-08-18 | AI Agent | Hui Wang | Fixed review gaps: corrected priority scheduling semantics (higher value = higher priority), added /api/processors and /api/tasks/{id}/progress endpoints, implemented video Range playback and production face detection injection. |
| **V0.3.1** | 2026-08-17 | AI Agent | Hui Wang | Added `build.py --action dev` one-click startup, configurable `FFC_WEB_PORT`/`FFC_WEB_HOST`, and frontend WebSocket auto-reconnect with exponential backoff. |
| **V0.3.0** | 2026-08-17 | AI Agent | Hui Wang | M4 landing: implemented FrameExtractor, FaceSelector, /api/faces detection and annotation, and reference face configuration. |
| **V0.2.0** | 2026-08-14 | AI Agent | Hui Wang | Integrated confirmed functional requirements (F1-F6), added TaskScheduler queue model, /api/faces and /media/ routes. |
| **V0.1.0** | 2026-08-14 | AI Agent | Hui Wang | Initial design draft: confirmed single-binary embedded server architecture, Drogon, React + Vite + TS. |

---

## 1. Background & Goals

FaceFusionCpp originally provided a CLI entry point. To lower the barrier to entry, a Web interface is provided: users can upload media, configure processors, submit tasks, and view real-time progress and results in the browser.

**Core Goals**:
1. Maintain the "single binary, zero environment configuration" delivery principle — `ffc --web` works out of the box;
2. Fully reuse the C++ core inference pipeline (`services.pipeline`) without rewriting business logic;
3. Maintain clear coexistence and independent evolution of the C++ codebase and frontend codebase in the same repository.

## 2. Decision Baseline

| Decision Point | Choice | Rationale |
| :--- | :--- | :--- |
| Deployment Form | Single-process embedded HTTP server | Preserves single-binary delivery with zero user setup |
| Frontend Stack | React + Vite + TypeScript | Mature ecosystem; outputs purely static assets |
| Build Integration | `build.py` unified entry (`--action web`) | One command for development and release, minimal CI change |
| HTTP Server | Drogon | All-in-one HTTP + WebSocket + static hosting; native WebSocket support |
| Progress Push | WebSocket | Bidirectional communication, ready for real-time progress push and future interactive preview |

### 2.1 Functional Requirements Matrix

| # | Feature | Implementation Notes | Workload |
| :--- | :--- | :--- | :--- |
| F1 | Image/Video Preview | Static media file API (`/media/...`) with HTTP Range / 206 support for video streaming | Low |
| F2 | Before/After Comparison | Frontend side-by-side / drag comparison of target and result files | Low |
| F3 | Batch Processing | Multiple source/target arrays in TaskConfig, multi-select upload in UI | Low |
| F4 | Task Queue & Priority | Task queue scheduler (FIFO + priority field, support promotion/demotion) | Medium |
| F5 | Video Frame Extraction | FrameExtractor component for single-frame extraction from video in browser | Low-Medium |
| F6 | Face Selection | Reuses mode + reference_face_path; `/api/faces` detection API for interactive canvas selection | Low-Medium |

## 3. Directory Layout (Single Repo, Dual Project)

```text
faceFusionCpp/
├── src/                                # C++ project
│   └── app/
│       ├── cli/                        # CLI entry (unchanged)
│       └── web/                        # Web server module (app.web)
│           ├── web_server.ixx/.cpp     # Drogon server lifecycle, static hosting, routes
│           ├── task_manager.ixx/.cpp   # Task queue and status state machine
│           └── pipeline_executor.ixx/.cpp # PipelineRunner integration
├── web/                                # Frontend project (npm workspace)
│   ├── src/                            # React source code
│   │   ├── pages/                      # Pages (TaskList, TaskCreate, TaskDetail)
│   │   ├── components/                 # Components (FileUploader, FaceSelector, FrameExtractor, ProgressBar)
│   │   ├── api/                        # REST client + WS client (auto-reconnect)
│   │   └── types/                      # TypeScript types aligned with C++ API
│   ├── package.json / package-lock.json
│   ├── vite.config.ts                  # Dev server proxy -> C++ backend
│   └── dist/                           # Build artifacts (gitignored)
├── assets/
│   └── web/                            # Frontend static assets packaged with binary
├── build.py                            # Supports --action web and --action dev
└── tests/
    ├── integration/app/                # web_server_test, web_ws_test, web_api_test
    └── e2e/scripts/web_api_test.py     # End-to-end test script
```

## 4. Architecture Design

### 4.1 Module Layering

`app.web` resides in the App layer: `app.web → services.pipeline → domain → foundation` (peer to `app.cli`, preserving existing layered architecture).

### 4.2 Runtime Architecture

```text
Browser (React SPA)
    │  HTTP (REST) / WebSocket
    ▼
Drogon (app.web module)
    ├── Static asset hosting: assets/web/
    ├── REST API: Task submission, query, cancel, faces, processors
    └── WebSocket: Task progress push (/ws/tasks/{id}/progress)
        │
        ▼
PipelineRunner (services.pipeline)
    ├── TaskConfig construction & validation
    └── TaskProgress callback -> WS broadcast
```

### 4.3 CLI Flags

```bash
./ffc --web [--web-port 8000] [--web-host 0.0.0.0] [--web-root <path>]
```
- Reuses global options (`--app-config`, `--log-level`);
- Mutually exclusive with quick mode (`-s/-t/-o`) and task config mode (`-c`);
- `--web-root`: frontend static root directory (defaults to `assets/web/`, can point to `web/dist` during dev).

### 4.4 REST API

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Service health check with version and status |
| `GET` | `/api/processors` | List of available processors and parameter schemas |
| `POST` | `/api/tasks` | Submit new task (JSON body matching TaskConfig) |
| `GET` | `/api/tasks` | List all tasks with status, queue position, priority |
| `GET` | `/api/tasks/{id}` | Task detail (media URLs, result files, parameters) |
| `GET` | `/api/tasks/{id}/progress` | Current task progress snapshot (WS disconnect compensation) |
| `POST` | `/api/tasks/{id}/priority` | Update task priority in queue |
| `POST` | `/api/tasks/{id}/cancel` | Cancel a running or queued task |
| `GET` | `/api/tasks/{id}/result` | Result files list |
| `POST` | `/api/upload` | Upload media file with `X-File-Name` header |
| `GET/POST` | `/api/faces` | Detect faces in image and return bounding boxes + landmarks |
| `GET` | `/media/{id}/{kind}/{name}` | Static media file serving with HTTP Range / 206 partial content |

### 4.5 WebSocket Protocol

- Endpoint: `/ws/tasks/{id}/progress`
- Server messages:
  - Progress: `{"type": "progress", "frame": 10, "total": 100, "fps": 28.5}`
  - Status: `{"type": "status", "status": "running"}`
  - Error: `{"type": "error", "message": "..."}`
- Automatic reconnection: Client reconnects with exponential backoff (1s -> 30s) and server replays current status/progress on connect.

### 4.6 Queue Model & Priority

```text
TaskManager (app.web)
├── Queue model: Priority Queue (higher integer value = higher priority, default 0)
├── Scheduling policy: Single concurrent task on GPU, FIFO within same priority
├── Priority adjustment: POST /api/tasks/{id}/priority re-orders queue
└── State machine: Queued -> Running -> Done / Cancelled / Failed
```

## 5. Build & Development

1. **One-click development**: `python build.py --action dev [--web-port 8000]`
2. **Build frontend**: `python build.py --action web`
3. **Build C++ & Web together**: `python build.py --action build`
