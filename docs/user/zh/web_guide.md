# Web 界面使用指南 (Web UI Guide)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-USER-ZH-WEBUI-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-17

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | 创建 Web 界面使用指南（M2：任务闭环 + 预览）。 |

## 1. 启动 Web 界面

在可执行文件所在目录运行：

**Linux (Bash)**:
```bash
./ffc --web
```

**Windows (PowerShell)**:
```powershell
.\ffc.exe --web
```

默认监听 `http://0.0.0.0:8000`，浏览器访问 `http://127.0.0.1:8000`。

常用参数：

| 参数 | 说明 | 默认值 |
| :--- | :--- | :--- |
| `--web-port` | 服务端口 | `8000` |
| `--web-host` | 绑定地址 | `0.0.0.0` |
| `--web-root` | 前端静态资源目录（开发调试可指向 `web/dist`） | `assets/web` |

> [!NOTE]
> `--web` 与快捷模式（`-s/-t/-o`）及任务配置模式（`-c`）互斥。

## 2. 页面说明

### 2.1 任务列表

- 展示全部任务（ID、状态、进度、素材数）；
- 每 3 秒自动刷新；运行中的任务显示实时进度条；
- 支持取消排队/运行中的任务。

### 2.2 提交任务

- **源人脸图片路径**：每行一个（或逗号分隔）；
- **目标图片/视频路径**：每行一个，支持批量（单任务多素材）；
- **处理器**：勾选 `face_swapper`、`face_enhancer`、`expression_restorer`、`frame_enhancer`；
- 输出目录可留空（使用默认配置）。

### 2.3 任务详情

- 实时进度（WebSocket 推送：帧数、FPS）；
- 取消任务；
- 完成后展示**处理前后对比**（左右）与结果文件列表（可下载）。

## 3. API 摘要

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 健康检查（含版本） |
| `POST` | `/api/tasks` | 提交任务（JSON：source_paths/target_paths/output_path/processors） |
| `GET` | `/api/tasks` | 任务列表 |
| `GET` | `/api/tasks/{id}` | 任务详情（含素材/结果 URL） |
| `POST` | `/api/tasks/{id}/cancel` | 取消任务 |
| `GET` | `/api/tasks/{id}/result` | 结果文件列表 |
| `WS` | `/ws/tasks/{id}/progress` | 实时进度推送 |
| `GET` | `/media/{task_id}/{kind}/{name}` | 素材/结果文件访问（白名单） |

## 4. 开发调试

- 前端开发：`cd web && npm run dev`（Vite dev server :5173，热更新），API/WS 自动代理到 `ffc --web`（:8000）；
- 集成调试：`npm run build` 后以 `./ffc --web --web-root web/dist` 直接托管产物；
- 重新构建前端产物：`python build.py --action web`。
