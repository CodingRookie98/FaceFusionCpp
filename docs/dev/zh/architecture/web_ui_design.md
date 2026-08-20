# Web 界面（Web UI）设计规格

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-ARCH-WEBUI-2026
> - **当前版本 (Version)**: V0.4.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V0.4.0** | 2026-08-20 | AI Agent | 王辉 | Web UI 全新重构设计：确立 Deep Studio Dark 沉浸式单屏工作台架构、多处理器实例动态管线、WYSIWYG 人脸交互点选映射、多模态对比矩阵（卷帘/放大镜/同步播放）、HUD 遥测与本地历史持久化，扩展 /api/tasks 支持 pipeline_steps 数组。 |
| **V0.3.2** | 2026-08-18 | AI Agent | 王辉 | 依据复核报告完成修复：修正 priority 调度语义（数值大优先）、落地 /api/processors 与 /api/tasks/{id}/progress 接口、落地视频 Range 与生产人脸检测注入。 |
| **V0.3.1** | 2026-08-17 | AI Agent | 王辉 | 更新 5.2 开发与生产：新增 `build.py --action dev` 一键启动、`FFC_WEB_PORT`/`FFC_WEB_HOST` 端口可配、前端 WebSocket 自动重连（指数退避）。 |
| **V0.3.0** | 2026-08-17 | AI Agent | 王辉 | M4 落地：实现 FrameExtractor 视频截帧、FaceSelector 人脸点选、/api/faces 检测标注与参考人脸配置。 |
| **V0.2.0** | 2026-08-14 | AI Agent | 王辉 | 整合用户确认的 6 项功能需求（F1-F6）：预览/对比/批量/队列优先级/视频帧/人脸选择；新增 `--web-root` 开发调试解耦机制、TaskScheduler 队列模型、/api/faces 与 /media/ API；里程碑扩展为 M1-M5。 |

> 仅保留最近 5 条记录，更早的历史可通过 git log 查阅。

## 1. 背景与目标

FaceFusionCpp 提供了 CLI 与基于 Drogon 的 Web 服务入口。前期 M1-M4 阶段验证了任务生命周期闭环、排队调度与人脸检测 API。为了提供媲美专业数字创意软件（如 DaVinci、Figma、ComfyUI）的一流交互体验，全面重构 Web 界面为 **Studio Workbench（沉浸式工作台）**。

**核心目标**:
1. 保持"单二进制、零环境配置"的交付理念——运行 `ffc --web` 即开即用；
2. **沉浸式单屏工作台**：消除页面割裂跳转，在一个高度集成的暗色工作台中完成“素材管理 ➔ 交互人脸映射 ➔ 动态管线编排 ➔ 实时多模态对比 ➔ 任务调度”全流程；
3. **多处理器实例管线**：支持在单次任务中按需挂载多个同类型（如多个不同参考人脸的 `face_swapper`）或不同类型的处理器实例；
4. **所见即所得 (WYSIWYG) 人脸交互**：画布直接框选、点选标注人脸并一键绑定为管线参考人脸；
5. **多模态对比矩阵**：支持卷帘滑动分割对比 (Split-Slider)、局部高倍放大镜 (Loupe) 以及视频同频对比播放。

## 2. 决策基线

| 决策点 | 选择 | 理由 |
| :--- | :--- | :--- |
| 部署形态 | 单进程内嵌 HTTP 服务 | 维持单二进制交付，用户零配置 |
| 前端技术栈 | React 19 + Vite + TypeScript + Tailwind CSS | 生态成熟，零多余体积，极高定制自由度与轻量静态打包 |
| 视觉设计语言 | Deep Studio Dark (专业暗色创意工作室) | 暗室环境最能还原多媒体图像/视频细节与色彩，专业质感 |
| 交互范式 | 3 栏沉浸式工作台 + 底部浮动 HUD | 视线流：左(素材池) ➔ 中(画布/对比) ➔ 右(管线/参数) ➔ 底(遥测/队列) |
| 构建集成 | `build.py` 统一入口（`--action web`） | 开发与发布一条命令，CI 改动最小 |
| HTTP Server | Drogon | 单库全包 HTTP + WebSocket + 静态托管；原生 WebSocket 支持 |
| 进度推送 | WebSocket | 双向通信与低延迟帧进度广播 |

### 2.1 功能需求清单 (V2 重构升级)

| # | 功能 | 模块/说明 | 状态/计划 |
| :--- | :--- | :--- | :--- |
| F1 | 图片/视频预览与 Range 播放 | 静态文件访问 API（/media/...），视频支持 HTTP Range | 已落地 (M2) |
| F2 | 处理前后对比（左右/并排） | 基础双图并排展示 | 已落地 (M2) |
| F3 | 批量任务 | TaskConfig 支持多 source/target 数组 | 已落地 (M3) |
| F4 | 任务排队 + 优先级调度 | TaskScheduler FIFO + priority 调度（数值大优先） | 已落地 (M3) |
| F5 | 视频帧单帧提取 | `<video>` 时间轴快速截帧并自动入池 | 已落地 (M4) |
| F6 | 人脸检测与标注 | `/api/faces` 检测人脸框与关键点 | 已落地 (M4) |
| **F7** | **Studio 沉浸式工作台** | 3 栏式响应式工作区布局，单屏聚合所有高频操作 | **重构新增 (M5)** |
| **F8** | **多处理器实例动态管线** | 支持同一管线添加多个同类型 Processor，自由命名、排序与调参 | **重构新增 (M5)** |
| **F9** | **画布 WYSIWYG 人脸映射** | 主画布直接绘制人脸高亮框，点击即可绑定对应步骤的 Reference Face | **重构新增 (M5)** |
| **F10**| **多模态对比矩阵** | 卷帘分割滑块 (Split-Slider)、局部细节放大镜 (Loupe)、视频同频播放 | **重构新增 (M5)** |
| **F11**| **实时遥测 HUD 与历史持久化** | 底部常驻 HUD（FPS/帧进度/队列），LocalStorage 历史任务一键回放与参数复用 | **重构新增 (M5)** |

## 3. 目录组织与前端工程结构

```
faceFusionCpp/
├── src/                                # C++ 工程
│   └── app/
│       └── web/                        # Web 服务模块
│           ├── web_server.ixx/.cpp     # Drogon 封装、静态托管、API 路由（扩展 pipeline_steps 解析）
│           ├── task_manager.ixx/.cpp   # 任务队列调度与执行管理
│           └── pipeline_executor.ixx/.cpp # 管线执行桥接
├── web/                                # 前端工程 (React 19 + TypeScript + Vite + Tailwind CSS)
│   ├── src/
│   │   ├── api/                        # API client, WS client, TS 接口定义 (types.ts)
│   │   ├── components/
│   │   │   ├── canvas/                 # 主画布组件群（人脸框渲染、WYSIWYG 点选、对比视口）
│   │   │   │   ├── ViewportCanvas.tsx  # 主视口容器
│   │   │   │   ├── FaceOverlay.tsx     # 人脸框与属性标签层
│   │   │   │   ├── SplitSlider.tsx     # 卷帘滑动分割对比组件
│   │   │   │   └── DetailLoupe.tsx     # 局部细节放大镜
│   │   │   ├── media/                  # 素材资产组件群
│   │   │   │   ├── AssetPool.tsx       # 左侧素材池（源人脸/目标素材/Sample 样例）
│   │   │   │   └── VideoFrameGrabber.tsx # 视频时间轴截帧工具
│   │   │   ├── pipeline/               # 处理器管线编排组件群
│   │   │   │   ├── PipelineEditor.tsx  # 右侧管线编辑器容器
│   │   │   │   ├── PipelineStepCard.tsx # 单个处理器实例卡片（开关/排序/调参）
│   │   │   │   └── PresetSelector.tsx  # 场景预设选择器
│   │   │   ├── hud/                    # 底部遥测与任务调度 HUD
│   │   │   │   ├── TelemetryHUD.tsx    # 实时进度、FPS、排队管理常驻栏
│   │   │   │   └── HistoryModal.tsx    # 历史任务与对比成果弹窗
│   │   │   └── ui/                     # 原子通用 UI 组件（Button, Slider, Switch, Badge 等）
│   │   ├── store/                      # 工作台状态管理（素材、管线、当前任务、视口模式）
│   │   ├── App.tsx                     # Studio 主入口容器
│   │   ├── index.css                   # Tailwind 指令与 Deep Studio Dark 主题变量
│   │   └── main.tsx
│   ├── package.json
│   ├── vite.config.ts
│   └── dist/
├── assets/
│   └── web/                            # 前端构建产物（被 install 打包并内嵌分发）
├── build.py                            # --action web / dev / build
└── docs/dev/zh/architecture/web_ui_design.md
```

## 4. C++ 架构设计与 API 契约扩展

### 4.1 模块位置与分层
`app.web` 位于 app 层，依赖方向：`app.web → services.pipeline → domain → foundation`。

### 4.2 REST API 规格扩展

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 服务健康检查与版本信息 |
| `GET` | `/api/processors` | 处理器元数据及可配置参数清单（供前端动态渲染表单滑块） |
| `POST` | `/api/tasks` | **【增强】** 提交任务，支持 `pipeline_steps` 结构化多步骤数组（向下兼容 `processors`） |
| `GET` | `/api/tasks` | 获取所有任务列表与队列状态 |
| `GET` | `/api/tasks/{id}` | 获取单个任务详细信息与结果文件列表 |
| `POST` | `/api/tasks/{id}/cancel` | 取消排队或执行中的任务 |
| `POST` | `/api/tasks/{id}/priority` | 调整任务在队列中的优先级（数值大优先） |
| `GET` | `/api/tasks/{id}/progress` | 查询任务最新进度 |
| `GET` | `/api/faces` | 图像人脸检测与关键点标注（返回 bbox, score, gender, age, kps） |
| `POST` | `/api/upload` | 本地素材临时上传 |
| `GET` | `/media/...` | 静态媒体文件直出与 HTTP Range 视频流支持 |

### 4.3 增强版任务创建请求结构 (JSON Payload)

```json
{
  "source_paths": [
    "assets/standard_face_test_images/lenna.bmp",
    "assets/standard_face_test_images/avatar_man.png"
  ],
  "target_paths": [
    "assets/standard_face_test_images/girl.bmp"
  ],
  "output_path": "build/bin/linux-x64-debug/outputs",
  "pipeline_steps": [
    {
      "step": "face_swapper",
      "name": "主角人脸替换",
      "enabled": true,
      "params": {
        "model": "inswapper_128_fp16",
        "face_selector_mode": "reference",
        "reference_face_path": "assets/standard_face_test_images/lenna.bmp"
      }
    },
    {
      "step": "face_swapper",
      "name": "配角人脸替换",
      "enabled": true,
      "params": {
        "model": "inswapper_128",
        "face_selector_mode": "reference",
        "reference_face_path": "assets/standard_face_test_images/avatar_man.png"
      }
    },
    {
      "step": "face_enhancer",
      "name": "高清细节增强",
      "enabled": true,
      "params": {
        "model": "codeformer",
        "blend_factor": 0.85,
        "face_selector_mode": "many"
      }
    },
    {
      "step": "expression_restorer",
      "name": "自然微表情还原",
      "enabled": true,
      "params": {
        "model": "live_portrait",
        "restore_factor": 0.6
      }
    }
  ]
}
```

后端 `web_server.cpp` 在解析时：若检测到 `pipeline_steps` 字段，直接迭代该数组，对每一个步骤应用 `ApplyCliParamsToStep` 构建 `config::PipelineStep` 结构并加入 `TaskConfig.pipeline` 中；若未提供则平滑回退到旧版 `processors` 字段解析。

## 5. 前端架构与视觉规范

### 5.1 Deep Studio Dark 视觉系统 (Design Tokens)

*   **Surfaces**:
    *   `bg-studio-canvas`: `#090d16`
    *   `bg-studio-panel`: `#0f172a`
    *   `bg-studio-card`: `#1e293b`
    *   `bg-studio-card-active`: `#26354a`
*   **Borders & Lines**:
    *   `border-studio-subtle`: `rgba(255, 255, 255, 0.08)`
    *   `border-studio-highlight`: `rgba(59, 130, 246, 0.5)`
*   **Accents**:
    *   `accent-blue`: `#3b82f6` (Primary Action)
    *   `accent-cyan`: `#06b6d4` (Running State)
    *   `accent-emerald`: `#10b981` (Completed)
    *   `accent-rose`: `#f43f5e` (Reference Lock)

### 5.2 Studio 三栏响应式拓扑

*   **左侧：素材资产池 (`AssetPool`, 280px)**
    *   源人脸图库（支持拖拽上传、多选、内置 Lenna 等测试样张一键载入）；
    *   目标素材池（图片/视频缩略图列表、快速截帧入口）；
    *   选定项即时投射到中央画布。
*   **中央：交互主画布与对比视口 (`ViewportCanvas`, Flex 1)**
    *   **编辑模式**：渲染当前选中的素材图像，叠加交互式人脸标注框（Bounding Box），显示人脸序号、置信度、性别与年龄；点击人脸框触发绑定事件；
    *   **对比模式**：任务完成后自动无缝激活对比视口：
        *   **卷帘分割 (Split-Slider)**：拖动滑块无级对比处理前/处理后；
        *   **局部放大镜 (Detail Loupe)**：光标悬停时弹出 2x-4x 局部细节放大镜，审查融合边缘与皮肤细节；
        *   **视频同频播放 (Synced Video)**：双视频同频 Seek / 播放。
*   **右侧：动态管线编排器 (`PipelineEditor`, 340px)**
    *   **预设选择器**：一键切换“极速单人换脸”、“高清写真重塑”、“多人精准换脸”、“影视级全链路”；
    *   **步骤列表卡片**：支持点击 `[＋ 添加处理器]` 添加任意步骤，每个步骤包含：独立名称、启用开关、上移/下移、删除、模型下拉框、参考人脸绑定指示器、浮点参数滑块（双向绑定）；
    *   **底部操作区**：`[🚀 启动处理]` 按钮（附带快捷键 `Ctrl+Enter`）。
*   **底栏：实时遥测与任务调度 HUD (`TelemetryHUD`, 固定底部)**
    *   常驻微型进度条、当前帧/总帧数、实时 FPS、状态指示灯；
    *   排队任务气泡、优先级提升/降低快捷按钮；
    *   历史记录抽屉入口（查看历史产物、一键重新加载参数）。

## 6. 构建与开发工作流

- **开发热重载**：`python build.py --action dev` 一键启动 C++ 后端与 Vite dev 代理；
- **前端测试**：`cd web && npm run test` (Vitest)；
- **前端打包**：`python build.py --action web` 编译并同步到 `assets/web/`；
- **集成测试与回归**：`python build.py --action test --test-label unit`。

## 7. 里程碑规划 (Milestones)

1. **M1~M4 基石阶段**：Drogon 服务、REST/WS 闭环、任务队列与优先级、视频截帧与人脸检测 API； ✅ **全部已完成**
2. **M5 Web Studio 全新重构阶段**：
   - **M5.1 基础脚手架**：Tailwind CSS / Lucide / Deep Studio Dark 样式基座搭建；
   - **M5.2 C++ API 升级**：`web_server.cpp` 扩充 `pipeline_steps` 数组解析与单元测试；
   - **M5.3 动态管线与资产组件**：实现多实例卡片、预设系统、素材资产池；
   - **M5.4 画布与多模态对比**：实现人脸交互点选、卷帘分割 Slider、局部放大镜 Loupe；
   - **M5.5 遥测 HUD 与历史持久化**：常驻底部 HUD、IndexedDB/LocalStorage 历史回放；
   - **M5.6 质量验收与打包交付**：单元测试、端到端功能验证、文档与安装包打包。

---
*关于分层结构的更多信息，请参阅 [layers.md](./layers.md)。*
