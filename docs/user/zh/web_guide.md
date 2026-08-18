# Web 界面使用指南 (Web UI Guide)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-USER-ZH-WEBUI-2026
> - **当前版本 (Version)**: V1.2.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-18

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.2.0** | 2026-08-18 | AI Agent | 王辉 | 增补动态处理器参数配置、视频 Range 流媒体播放、/api/processors 与 /api/tasks/{id}/progress 接口及一键开发模式说明。 |
| **V1.1.0** | 2026-08-17 | AI Agent | 王辉 | 增补 M4 视频帧截取（FrameExtractor）、人脸检测标注（FaceSelector）与参考人脸配置说明。 |
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

- 展示全部任务（ID、状态、进度、素材数、优先级、排队位置）；
- 每 3 秒自动刷新；运行中的任务显示实时进度条；
- 支持取消排队/运行中的任务，支持提升/降低排队任务的优先级（数值越大优先级越高）。

### 2.2 提交任务

- **上传文件**：源/目标输入框上方提供上传按钮，支持多文件选择（图片/视频），上传后自动填入服务端路径；
- **📹 视频帧提取工具 (FrameExtractor)**：展开工具面板，选择本地视频后可自由拖动进度条、微调时间点（±0.1s / ±1s），一键截取当前视频帧并上传为素材路径；
- **👤 人脸检测与点选 (FaceSelector)**：点击“🔍 检测源图人脸”，系统调用 `/api/faces` 检测人脸并可视化叠加检测框，点击即可直接选中特定人脸；
- **人脸选择策略 (Face Selector Mode)**：
  - **全部人脸 (Many)**：默认替换目标画面中检测到的所有面孔；
  - **单个最高分人脸 (One)**：仅替换画面中检测置信度最高的单个人脸；
  - **参考人脸比对 (Reference)**：可指定参考人脸路径或从源图中点选，仅替换与参考人脸相似度最高的面孔；
- **处理器与动态参数配置**：
  - 勾选 `face_swapper`、`face_enhancer`、`expression_restorer`、`frame_enhancer`；
  - 勾选后自动根据 `/api/processors` 导出的元数据渲染对应的动态参数表单（如模型选择、融合比例 `blend_factor`、恢复因子 `restore_factor`、增强倍率等）；
- 输出目录可留空（使用默认配置）。

### 2.3 任务详情

- 实时进度（WebSocket 推送：帧数、FPS）；
- 取消任务；
- 完成后展示**处理前后对比**（图片并排/拖拽对比，视频支持原生 `<video>` Range 流式播放）与结果文件列表（可下载）。

## 3. API 摘要

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 健康检查（含版本与状态） |
| `GET` | `/api/processors` | 处理器列表与参数模式元数据 |
| `POST` | `/api/tasks` | 提交任务（JSON：source_paths/target_paths/output_path/processors/processor_params） |
| `GET` | `/api/tasks` | 任务列表（含排队位置与优先级） |
| `GET` | `/api/tasks/{id}` | 任务详情（含素材/结果 URL） |
| `GET` | `/api/tasks/{id}/progress` | 任务当前进度快照（WS 断线补偿） |
| `POST` | `/api/tasks/{id}/cancel` | 取消任务 |
| `POST` | `/api/tasks/{id}/priority` | 修改优先级（仅排队中任务） |
| `POST` | `/api/upload` | 上传文件（二进制 body + `X-File-Name` 头） |
| `POST`/`GET` | `/api/faces` | 图像人脸检测与标注（返回人脸框、置信度、年龄/性别、关键点） |
| `GET` | `/api/tasks/{id}/result` | 结果文件列表 |
| `WS` | `/ws/tasks/{id}/progress` | 实时进度推送 |
| `GET` | `/media/{task_id}/{kind}/{name}` | 素材/结果文件访问（支持 HTTP Range / 206 视频流） |

## 4. 开发调试

- **一键启动**：`python build.py --action dev [--web-port 8000]`（自动后台拉起 C++ 服务与 Vite 热更新前端）；
- **前端开发**：`cd web && npm run dev`（Vite dev server :5173，热更新），API/WS 自动代理到 `ffc --web`（:8000）；
- **集成调试**：`npm run build` 后以 `./ffc --web --web-root web/dist` 直接托管产物；
- **重新构建前端产物**：`python build.py --action web`。
