# Web UI Guide

> **Document Control**
> - **Document ID**: FFC-USER-EN-WEBUI-2026
> - **Version**: V2.3.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-21

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V2.3.0** | 2026-08-21 | AI Agent | 王辉 | Added 【⚡ Instant Preview Render】 single-frame effect verification and video pause-frame extraction; streamlined 【➕ Add to Queue】 CTA; implemented per-task output sandbox directory isolation (`./output/<uuid>`); standardized compare disabled on raw material and active after preview render. |
| **V2.2.1** | 2026-08-21 | AI Agent | 王辉 | Added media preview lifecycle persistence & recovery documentation (seamless /api/preview fallback upon page reload, auto face detection recovery on active target media). |
| **V2.2.0** | 2026-08-21 | AI Agent | 王辉 | Upgraded task queue & omni-preview architecture: renamed submit CTA to "Add to Task Queue", left sidebar 3-tab layout (Source Assets / Target Media / Task Queue), priority up/down & cancel actions on task cards, and omni canvas preview for source/target/queue items. |
| **V2.1.0** | 2026-08-21 | AI Agent | 王辉 | Added multi-file concurrent & drag-and-drop uploads, multi-face selection/toggle with batch controls, standalone result preview with download action, and smart split-slider matching. |
| **V2.0.0** | 2026-08-20 | AI Agent | 王辉 | Completely overhauled for Studio Workbench: 3-column responsive layout, multi-instance dynamic pipeline, WYSIWYG face overlay, split-slider/loupe comparison matrix, and live telemetry HUD. |
| **V1.2.0** | 2026-08-18 | AI Agent | 王辉 | Added dynamic processor parameters, video Range streaming playback, /api/processors, /api/tasks/{id}/progress, and one-click dev mode. |

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

Default listen address `http://0.0.0.0:8000` (configured via `app.yaml`); open `http://127.0.0.1:8000` in a browser.

Common options:

| Option | Description | Default |
| :--- | :--- | :--- |
| `--web-port` | Server port (defaults to `web.port` from `app.yaml`) | `8000` |
| `--web-host` | Bind address (defaults to `web.host` from `app.yaml`) | `0.0.0.0` |
| `--web-root` | Frontend assets root (defaults to `web.web_root` from `app.yaml`) | `assets/web` |

> [!NOTE]
> `--web` is mutually exclusive with quick mode (`-s/-t/-o`) and task config mode (`-c`).

## 2. Studio Workbench Overview

Web UI provides an integrated, darkroom-styled **Deep Studio Dark single-page workbench**:

### 2.1 Left Sidebar: Asset Pool & Task Queue
The left sidebar aggregates 3 primary tabs:
- **Source Assets (Sources)**: Manages all source references with multi-file concurrent and drag-and-drop uploads; clicking any item instantly previews it in high definition on the center canvas;
- **Target Media (Targets)**: Upload and organize target images or video assets; clicking projects onto the canvas with automatic face detection & interactive overlay;
- **Task Queue**: Real-time list of all queued, running, done, and failed tasks. Each card provides:
  - 【**⬆️ Promote Priority**】: Raise task queue priority for earlier worker scheduling (available when pending);
  - 【**⬇️ Demote Priority**】: Lower task queue priority (available when pending);
  - 【**✕ Cancel Task**】: Cancel a pending or running task;
  - **Live Canvas Binding**: Clicking any task card synchronizes the center viewport to display the task's base target, live progress HUD, or finished results.
  - **Per-Task Output Isolation**: Each task's output files are isolated in its dedicated sandbox subdirectory (`./output/<uuid>/`), preventing result collisions.

### 2.2 Center Viewport: Omni Viewport Canvas & Comparison Matrix
- **Omni Perception Modes**:
  - **Source Material Mode**: High-resolution view of source image/video (SplitSlider comparison disabled for raw inputs);
  - **Target Media Mode**: Target image canvas with WYSIWYG bounding box overlay (Red `✓ Selected` $\leftrightarrow$ Cyan `○ Unselected`) with batch select/clear toolbar;
  - **Task Queue Mode**: Live frame progress HUD, queue position indicators, or completed output viewer.
- **Multimodal Viewport Tools**:
  - **Result Viewer**: Standalone full-res inspector with a direct 【**Download Result**】 button;
  - **Compare (SplitSlider)**: Enabled after task completion or 【Instant Preview Render】, drag divider to compare before & after;
  - **Detail Loupe**: High-power 2.8x magnifying lens for pixel-level inspection of eyes, hair blending, and skin texture.

### 2.3 Right Sidebar: Pipeline Editor & Dual CTAs
- **Official Presets**: Fast 1-click presets ("Fast Single Swap", "Portrait Remaster", "Multi-Face Swap", "Cinematic Full Upscale");
- **Multi-Instance Pipeline**: Chain multiple processor steps of same/different types with custom bindings;
- **Dual Core Action CTAs**:
  - 【**⚡ Instant Preview Render**】: Single-frame instant execution (does not enter queue). For video targets, becomes active when paused to grab the current frame, perform detection and single-frame swap, and switch into compare mode;
  - 【**➕ Add to Queue**】: Package the task into background worker queue for asynchronous batch processing.

### 2.4 Bottom Bar: Telemetry & Task Scheduler HUD
- **Live Telemetry**: C++ Core online state, current frame progress, live FPS;
- **Queue Controls**: View queue length and adjust priorities on the fly;
- **History Drawer**: Access past completions with one click.

## 3. REST API Contract Summary

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Backend status & version |
| `GET` | `/api/processors` | Available processors & parameter schema |
| `POST` | `/api/preview_render` | Lightweight single-frame instant rendering (supports base64 frame) |
| `POST` | `/api/tasks` | Submit task (allocates isolated output directory `./output/<uuid>`) |
| `GET` | `/api/tasks` | List all tasks with queue status |
| `GET` | `/api/tasks/{id}` | Task detail and output file list |
| `POST` | `/api/tasks/{id}/cancel` | Cancel queued or running task |
| `POST` | `/api/tasks/{id}/priority` | Adjust queue priority |
| `GET` | `/api/tasks/{id}/progress` | Real-time frame progress |
| `POST` | `/api/faces` | Face detection & landmark analysis |
| `POST` | `/api/upload` | Upload media file to temp directory |
| `GET` | `/media/...` | Serve static media and processed outputs (Range supported) |
