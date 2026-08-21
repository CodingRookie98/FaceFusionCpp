# Web User Interface (Web UI) Design Specification

> **Document Control Information**
> - **Document ID**: FFC-DEV-EN-ARCH-WEBUI-2026
> - **Current Version**: V0.5.1
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: Hui Wang
> - **Reviewer**: Hui Wang
> - **Last Updated**: 2026-08-21

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V0.5.1** | 2026-08-21 | AI Agent | Hui Wang | Specified media preview lifecycle persistence (smart Blob fallback to backend /api/preview, localStorage sanitization) and automatic face detection recovery upon page reload. |
| **V0.5.0** | 2026-08-21 | AI Agent | Hui Wang | Upgraded task queue & omni-preview architecture: renamed submit CTA to "Add to Task Queue", left sidebar 3-tab layout (Source Assets / Target Media / Task Queue), priority up/down & cancel actions on task cards, and omni canvas preview for source/target/queue items. |
| **V0.4.1** | 2026-08-21 | AI Agent | Hui Wang | Added multi-file concurrent & drag-and-drop uploads, multi-face selection/toggle with batch controls, standalone result preview with download action, C++ Web structured logging, and Playwright E2E test suite. |
| **V0.4.0** | 2026-08-20 | AI Agent | Hui Wang | Completely redesigned for Studio Workbench: Deep Studio Dark single-page layout, dynamic multi-instance processor pipeline, WYSIWYG face mapping, split-slider/loupe comparison matrix, telemetry HUD, and /api/tasks pipeline_steps. |
| **V0.3.2** | 2026-08-18 | AI Agent | Hui Wang | Fixed review gaps: corrected priority scheduling semantics (higher value = higher priority), added /api/processors and /api/tasks/{id}/progress endpoints, implemented video Range playback and production face detection injection. |

---

## 1. Background & Goals

FaceFusionCpp originally provided a CLI entry point. To deliver a first-class user experience comparable to modern creative suites (DaVinci, Figma, ComfyUI), the Web interface is overhauled into **Studio Workbench**: an integrated single-page darkroom environment where media asset management, interactive face mapping, dynamic pipeline orchestration, real-time comparison, and task scheduling converge seamlessly.

**Core Goals**:
1. Maintain the "single binary, zero environment configuration" delivery principle — `ffc --web` works out of the box;
2. **Deep Studio Dark Single-Page Workbench**: 3-column unified layout eliminating disconnected navigation;
3. **Multi-Instance Dynamic Pipeline**: Mount multiple instances of any processor (e.g. separate `face_swapper` steps with individual reference faces);
4. **WYSIWYG Face Mapping & Multi-Selection**: Bounding box rendering, click-to-toggle multi-face selection, select-all/clear controls, and automatic pipeline parameter synchronization;
5. **Omni Canvas Perception Preview**: Clicking any item in Source Assets, Target Media, or Task Queue immediately previews the respective media, face overlay, or task render/progress on the center canvas;
6. **Dynamic Task Queue Workbench**: Direct enqueueing ("Add to Task Queue"), left sidebar task queue cards with real-time priority promotion/demotion and one-click task cancellation.

## 2. Decision Baseline

| Decision Point | Choice | Rationale |
| :--- | :--- | :--- |
| Deployment Form | Single-process embedded HTTP server | Preserves single-binary delivery with zero user setup |
| Frontend Stack | React 19 + Vite + TypeScript + Tailwind CSS | Mature ecosystem; outputs purely static assets |
| Visual Design Language | Deep Studio Dark (Creative Studio Aesthetic) | Darkroom environment provides optimal color and detail inspection |
| Interaction Paradigm | 3-Column Studio + Floating Telemetry HUD | Natural visual flow: Left (Assets/Queue) ➔ Center (Omni Canvas) ➔ Right (Pipeline/Enqueue) ➔ Bottom (HUD) |
| Build Integration | `build.py` unified entry (`--action web`) | One command for development and release, minimal CI change |
| HTTP Server | Drogon | All-in-one HTTP + WebSocket + static hosting; native WebSocket support |
| Progress Push | WebSocket | Bidirectional communication, real-time frame progress push |
| Automated Testing | Vitest (Unit) + Playwright (Full-stack E2E) | Validates frontend state, components, and real C++ core inference pipeline |

### 2.1 Functional Requirements Matrix

| # | Feature | Module / Implementation | Status |
| :--- | :--- | :--- | :--- |
| F1 | Image/Video Preview | Static media API (`/media/...`) with HTTP Range for streaming | Implemented (M2) |
| F2 | Before/After Comparison | Side-by-side comparison | Implemented (M2) |
| F3 | Batch Processing | Multiple source/target arrays in TaskConfig | Implemented (M3) |
| F4 | Task Queue & Priority | TaskScheduler FIFO + priority field (higher = earlier) | Implemented (M3) |
| F5 | Video Frame Extraction | Frame grabber from `<video>` timeline into asset pool | Implemented (M4) |
| F6 | Face Detection & Annotation | `/api/faces` detection API for interactive canvas annotation | Implemented (M4) |
| **F7** | **Studio Single-Page Workbench** | 3-column responsive layout aggregating all high-frequency operations | **Implemented (M5)** |
| **F8** | **Dynamic Multi-Instance Pipeline**| Multiple instances of any processor with custom naming, order, and parameters | **Implemented (M5)** |
| **F9** | **WYSIWYG Face Multi-Select & Toggle** | Canvas bounding box multi-selection, toggle selection, Select All/Clear | **Implemented (M5)** |
| **F10**| **Multi-Modal Comparison & Result Viewer** | Split slider, standalone result viewer with download button, detail loupe | **Implemented (M5)** |
| **F11**| **Real-Time Telemetry HUD & History** | Persistent bottom dock with FPS/progress telemetry and LocalStorage history | **Implemented (M5)** |
| **F12**| **Multi-File Concurrent & Drag Upload** | Multi-file concurrent upload and drag-and-drop with live progress counters | **Implemented (M5)** |
| **F13**| **C++ Web Module Structured Logging** | Full lifecycle logging across HTTP/WS routes, uploads, detections, and tasks | **Implemented (M5)** |
| **F14**| **Playwright Full-Stack E2E Test Suite**| Automated end-to-end integration tests with real backend inference | **Implemented (M5)** |
| **F15**| **Add to Task Queue & Queue Panel** | Submit CTA renamed to "Add to Task Queue", left panel integrated task queue tab | **Implemented (M5)** |
| **F16**| **Queue Item Priority & Cancel Actions**| Task cards support ⬆️ Promote Priority, ⬇️ Demote Priority, ✕ Cancel Task | **Implemented (M5)** |
| **F17**| **Omni Canvas Perception Preview** | Seamless preview of Source Assets, Target Media, and Task Queue runs on canvas | **Implemented (M5)** |

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
