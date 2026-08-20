# Web 界面使用指南 (Web UI Guide)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-USER-ZH-WEBUI-2026
> - **当前版本 (Version)**: V2.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V2.0.0** | 2026-08-20 | AI Agent | 王辉 | 全面重构为 Studio Workbench 沉浸式创作工作台指南：3 栏响应式布局、多实例动态管线、WYSIWYG 画布人脸交互、卷帘/放大镜多模态对比、常驻遥测 HUD。 |
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

## 2. Studio Workbench 沉浸式工作台

Web UI V2 采用专为 AI 视觉创作打造的 **Deep Studio Dark 单屏工作台**：

### 2.1 左栏：素材资产池 (Asset Pool)
- **源人脸图库 (Sources)**：多选拖拽上传人脸素材，提供内置测试样张（Lenna / 男士头像）一键快速载入；
- **目标素材池 (Targets)**：上传目标图片或视频文件，选中后即时投射到中央主视口；
- **视频时间轴截帧**：视频素材自动支持播放与关键帧选取。

### 2.2 中栏：交互视口与主画布 (Viewport Canvas)
- **WYSIWYG 人脸点选标注**：选择目标图片后自动调用后台检测算法，在画布上渲染高亮人脸框、置信度、性别与年龄；直接点击人脸框即可将其绑定为管线中指定步骤的参考人脸；
- **多模态对比视口**：
  - **标注画布 (Canvas)**：素材预览与人脸交互点选；
  - **卷帘对比 (Split Slider)**：任务完成后左右无级拖动滑块对比处理前与处理后细节；
  - **局部放大镜 (Detail Loupe)**：光标悬停时弹出 2.5x 高倍放大镜审查眼部、发丝与肤质融合度；
  - **视频同频对齐**：原视频与生成视频同频对齐播放。

### 2.3 右栏：动态管线编排器 (Pipeline Editor)
- **官方智能预设**：一键切换“极速单人换脸”、“高清写真重塑”、“多人精准多脸替换”、“影视级全流程超分”；
- **多实例流水线**：支持在同一管线中添加多个同类型处理器（如多个独立配置参考人脸的 `face_swapper`）；
- **参数即时调节**：模型切换、模式选择、`blend_factor` / `restore_factor` / `enhance_factor` 强度滑块双向微调。

### 2.4 底栏：实时遥测与任务调度 HUD (Telemetry HUD)
- **实时遥测指标**：当前运行任务状态、帧进度百分比、实时推理 FPS；
- **队列调度与优先级**：查看排队任务序号，一键提升/降低排队优先级（P 值越大越优先调度），一键取消任务；
- **历史记录抽屉**：点击“历史成果”随时浏览历史完成任务，一键载入工作台比对。

## 3. REST API 契约摘要

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 健康检查（含版本与状态） |
| `GET` | `/api/processors` | 处理器列表与参数元数据 |
| `POST` | `/api/tasks` | 提交任务（支持 `pipeline_steps` 结构化多步骤数组） |
| `GET` | `/api/tasks` | 全部任务列表（含状态、进度与队列位置） |
| `GET` | `/api/tasks/{id}` | 任务详情（含结果文件列表） |
| `POST` | `/api/tasks/{id}/cancel` | 取消排队或执行中的任务 |
| `POST` | `/api/tasks/{id}/priority` | 调整任务在队列中的优先级 |
| `GET` | `/api/tasks/{id}/progress` | 查询任务最新进度 |
| `POST` | `/api/faces` | 素材人脸检测标注与关键点分析 |
| `POST` | `/api/upload` | 素材上传至临时目录 |
| `GET` | `/media/...` | 素材与结果文件访问（视频支持 HTTP Range 流媒体） |
