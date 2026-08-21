# Web UI Guide

> **Document Control**
> - **Document ID**: FFC-USER-EN-WEBUI-2026
> - **Version**: V2.1.0
> - **Status**: Official
> - **Authority**: Informative
> - **Owner**: 王辉
> - **Reviewer**: 王辉
> - **Last Updated**: 2026-08-21

## Revision History

| Version | Date | Author | Reviewer | Description |
| :--- | :--- | :--- | :--- | :--- |
| **V2.1.0** | 2026-08-21 | AI Agent | 王辉 | Added multi-file concurrent & drag-and-drop uploads, multi-face selection/toggle with batch controls, standalone result preview with download action, and smart split-slider matching. |
| **V2.0.0** | 2026-08-20 | AI Agent | 王辉 | Completely overhauled for Studio Workbench: 3-column responsive layout, multi-instance dynamic pipeline, WYSIWYG face overlay, split-slider/loupe comparison matrix, and live telemetry HUD. |
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

## 2. Studio Workbench Overview

Web UI V2 provides an integrated, darkroom-styled **Deep Studio Dark single-page workbench**:

### 2.1 Left Sidebar: Asset Pool
- **Multi-File Concurrent & Drag-and-Drop Upload**: Select and upload multiple JPG, PNG, BMP, MP4, MOV files simultaneously, or drag them directly into the asset dropzone; includes live upload progress feedback and error aggregation;
- **Source Faces**: Manage reference portrait images with built-in standard test samples (Lenna / Male Portrait / Female Portrait);
- **Target Media**: Upload and organize target images or video assets, automatically projected onto the central viewport with face detection;
- **Video Scrubbing**: Quick frame extraction from local videos into the asset pool.

### 2.2 Center: Viewport Canvas & Comparison Stage
- **WYSIWYG Face Multi-Selection & Toggle**:
  - **Default All Selected**: Automatic detection marks all found faces as selected upon media load;
  - **Click to Toggle Selection**: Click any face box or tag to toggle selection (red box `✓ Selected #N` vs cyan box `○ Unselected #N`);
  - **Batch Control Toolbar**: Real-time counter `Selected N/M Faces` with 【**Select All**】 and 【**Clear**】 quick buttons;
  - **Pipeline Sync**: Single selection sets `reference` mode; multi/all selection sets `many` mode with specific face index lists.
- **Multi-Modal Comparison Matrix**:
  - **Annotation Canvas**: Interactive face selection and bounding box inspection;
  - **Result Viewer**: Standalone full-image/video inspection of output assets, with a direct 【**Download Result**】 button;
  - **Split Slider**: Smooth Before / After comparison slider auto-activated upon task completion;
  - **Detail Loupe**: 2.8x high-magnification floating loupe to examine blending edges and skin texture;
  - **Synced Video**: Synchronized playback of original and processed video files.

### 2.3 Right Sidebar: Dynamic Pipeline Editor
- **Official Presets**: One-click switching for Fast Swap, HD Portrait, Multi-Face Swap, and Cinematic Remaster;
- **Multi-Instance Pipeline**: Mount multiple instances of any processor (e.g. multiple `face_swapper` steps with independent reference faces);
- **Interactive Tuning**: Model dropdowns, selection strategy switches, and live numeric sliders for blend factors.

### 2.4 Bottom Dock: Telemetry HUD
- **Real-Time Telemetry**: C++ Core status, active task frame progress percentage, live inference FPS;
- **Queue Scheduler**: Queue positions, priority controls (higher value = earlier execution), and task cancellation;
- **History Drawer**: Access past completed runs and reload them into the studio workbench with one click.

## 3. REST API Summary

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/api/health` | Service health check and version info |
| `GET` | `/api/processors` | List available processors and parameter schemas |
| `POST` | `/api/tasks` | Create task with structured `pipeline_steps` array |
| `GET` | `/api/tasks` | List all tasks with queue status and progress |
| `GET` | `/api/tasks/{id}` | Task details and output file URLs |
| `POST` | `/api/tasks/{id}/cancel` | Cancel a queued or running task |
| `POST` | `/api/tasks/{id}/priority` | Update task queue priority |
| `GET` | `/api/tasks/{id}/progress` | Query latest task progress |
| `POST` | `/api/faces` | Detect faces and keypoints on an image |
| `POST` | `/api/upload` | Upload media files to temporary directory |
| `GET` | `/media/...` | Serve static media and processed outputs (Range supported) |
