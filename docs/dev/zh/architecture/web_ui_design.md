# Web 界面（Web UI）设计规格

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-ARCH-WEBUI-2026
> - **当前版本 (Version)**: V0.2.0（草稿，待评审）
> - **状态 (Status)**: 评审中 (In Review)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V0.2.0** | 2026-08-14 | AI Agent | 王辉 | 整合用户确认的 6 项功能需求（F1-F6）：预览/对比/批量/队列优先级/视频帧/人脸选择；新增 `--web-root` 开发调试解耦机制、TaskScheduler 队列模型、/api/faces 与 /media/ API；里程碑扩展为 M1-M5。 |
| **V0.1.0** | 2026-08-14 | AI Agent | 王辉 | 依据设计讨论创建：确认部署形态、技术栈、构建集成、HTTP Server 与进度推送选型。 |

## 1. 背景与目标

FaceFusionCpp 目前仅提供 CLI 入口。为降低使用门槛，新增 Web 界面：用户在浏览器中上传素材、配置处理器、启动任务并实时查看进度与结果。

**核心目标**:
1. 保持"单二进制、零环境配置"的交付理念——运行 `ffc --web` 即开即用；
2. C++ 核心推理管线（`services.pipeline`）完全复用，不重写业务逻辑；
3. C++ 工程与前端工程在同一仓库内清晰共存、独立演进。

## 2. 决策基线（已确认）

| 决策点 | 选择 | 理由 |
| :--- | :--- | :--- |
| 部署形态 | 单进程内嵌 HTTP 服务 | 维持单二进制交付，用户零配置 |
| 前端技术栈 | React + Vite + TypeScript | 生态成熟，产物为纯静态文件 |
| 构建集成 | `build.py` 统一入口（`--action web`） | 开发与发布一条命令，CI 改动最小 |
| HTTP Server | Drogon | 单库全包 HTTP + WebSocket + 静态托管；原生 WebSocket 支持（进度推送需求） |
| 进度推送 | WebSocket | 双向通信，为未来实时参数调整/预览预留能力 |

> 注：cpp-httplib 因不支持 WebSocket 被否决；uWebSockets 为备选（依赖更轻但 API 偏底层）。

### 2.1 功能需求清单（用户确认）

| # | 功能 | 实现要点 | 主要工作量 |
| :--- | :--- | :--- | :--- |
| F1 | 图片/视频预览 | 静态文件访问 API（/media/...），视频需 HTTP Range 支持（Drogon 原生支持） | 小 |
| F2 | 处理前后对比（左右） | 纯前端：并排/拖拽对比原素材与结果文件 URL | 小 |
| F3 | 批量任务（单任务多图/视频） | TaskConfig 已支持多 source/target 数组，API 传数组 + 前端多选上传 | 小 |
| F4 | 任务排队 + 提升优先级 | 新增任务队列调度器（FIFO + priority 字段，支持提升/降级） | 中（架构性新增） |
| F5 | 视频帧单帧处理 | 阶段一：前端 `<video>` 截帧上传；阶段二（可选）：服务端 ffmpeg 抽帧 API | 小-中 |
| F6 | 选择人脸（face_selector） | 复用现有参数体系（mode + reference_face_path）；新增人脸检测标注 API（`/api/faces`）供前端点选 | 小-中 |

## 3. 目录组织（单一仓库，双工程）

```
faceFusionCpp/
├── src/                                # C++ 工程（现有，新增 web 模块）
│   └── app/
│       ├── cli/                        # 现有 CLI 入口（保持不变）
│       └── web/                        # 【新增】Web 服务模块
│           ├── web_server.ixx/.cpp     # Drogon 封装：生命周期/静态托管/路由
│           ├── ws_session.ixx/.cpp     # WebSocket 会话（任务进度推送）
│           └── handlers/               # REST 处理器（任务 CRUD）
├── web/                                # 【新增】前端工程（独立 npm 工程）
│   ├── src/                            # React 源码
│   │   ├── pages/                      # 页面（任务列表/任务配置/结果预览）
│   │   ├── components/                 # 组件（上传/处理器参数/进度条）
│   │   ├── api/                        # API client + WS client
│   │   └── types/                      # 与 C++ API 对齐的 TS 类型
│   ├── package.json / package-lock.json # 锁文件提交
│   ├── vite.config.ts                  # dev server 代理 → C++ 服务
│   └── dist/                           # 构建产物（gitignore）
├── assets/
│   └── web/                            # 【新增】前端产物落点（被 install 打包）
├── build.py                            # 新增 --action web
├── CMakeLists.txt                      # install 增加 assets/web
├── tests/
│   └── e2e/scripts/                    # 新增 web_api 场景
└── docs/user/{zh,en}/web_guide.md      # 新增用户文档
```

**管理原则**:
- `src/` 与 `web/` 严格隔离：C++ 侧无 JS 代码，前端只通过 HTTP/WS 与 C++ 通信；
- 版本统一：单一 `project.yaml` 版本号，前后端同仓库同 Tag 发布；
- 前端产物（`web/dist`、`assets/web`）不提交 git，由 `build.py --action web` 生成；
- `web/node_modules` 加入 .gitignore。

**开发/生产资源解耦机制（回应"内嵌资产不利于调试"的关切）**:
- C++ 静态托管根目录**可配置**：`--web-root <path>`（默认 `assets/web/`），也可经 `app_config.yaml` 的 `web.root` 配置；
- 开发期两条路径，均无需拷贝产物：
  1. 前端调试：Vite dev server（热更新）+ 代理 `/api`、`/ws` 到 C++ 服务；
  2. 集成调试：`vite build` 后以 `--web-root web/dist` 启动 C++ 服务直接托管；
- 生产：默认 `assets/web/` 随 install 打包，单二进制交付不变。

## 4. C++ 架构设计

### 4.1 模块位置与分层

`app.web` 位于 app 层，依赖方向：`app.web → services.pipeline → domain → foundation`（与 `app.cli` 平级，不破坏现有分层）。

### 4.2 运行时架构

```
浏览器 (React SPA)
    │  HTTP (REST) / WebSocket
    ▼
Drogon (app.web 模块)
    ├── 静态资源托管: assets/web/（前端产物）
    ├── REST API: 任务提交/查询/取消
    └── WebSocket: 任务进度推送
        │
        ▼
PipelineRunner（复用现有实现）
    ├── TaskConfig 构建（复用 config.parser/merger/validator）
    └── TaskProgress 回调 → WS 广播
```

### 4.3 CLI 入口扩展

```bash
./ffc --web [--web-port 8000] [--web-host 0.0.0.0] [--web-root <path>]
```
- 复用现有全局选项（`--app-config`、`--log-level` 等）；
- `--web` 与快捷模式（`-s/-t/-o`）及任务配置模式（`-c`）互斥（复用 CLI11 excludes 机制）；
- 无任务参数时启动 Web 模式；Web 模式下任务通过 API 提交；
- `--web-root`：前端静态资源根目录（默认 `assets/web/`，开发调试可指向 `web/dist`）。

### 4.4 REST API 草案

| 方法 | 路径 | 说明 |
| :--- | :--- | :--- |
| `GET` | `/api/health` | 服务健康检查（含 GPU/模型状态摘要，复用 system_check） |
| `GET` | `/api/processors` | 处理器列表与参数元数据（复用 ProcessorParamRegistry） |
| `POST` | `/api/tasks` | 提交任务（JSON 体：source/target/output/processors/参数，对齐 TaskConfig） |
| `GET` | `/api/tasks` | 任务列表（含状态） |
| `GET` | `/api/tasks/{id}` | 任务详情 |
| `POST` | `/api/tasks/{id}/cancel` | 取消任务（复用 ShutdownHandler 取消语义） |
| `POST` | `/api/tasks/{id}/priority` | 提升/降低优先级（F4 队列调度） |
| `GET` | `/api/tasks/{id}/result` | 结果文件信息（下载/预览 URL） |
| `GET` | `/api/tasks/{id}/progress` | 历史进度查询（WS 断线补偿） |
| `GET` | `/api/faces` | 素材人脸检测标注（F6：返回人脸框坐标供前端点选，复用 face analyser） |
| `GET` | `/media/...` | 素材/结果静态文件访问（F1/F2：仅限配置目录，视频 Range 支持） |

### 4.5 WebSocket 协议草案

- 端点：`/ws/tasks/{id}/progress`；
- 服务端消息：`{type: "progress", frame, total, fps, percent}`（数据源：TaskProgress 回调）；`{type: "done", result}` / `{type: "error", code, message}`；
- 客户端消息：`{type: "ping"}` 保活（或依赖 Drogon 心跳）。

### 4.6 任务队列与优先级（F4）

```
TaskScheduler（app.web 层新增，或 services 层复用扩展）
├── 队列模型: FIFO + priority（int，数值小优先；默认 0）
├── 调度策略: 单 GPU 单任务执行，其余排队等待
├── 优先级变更: POST /api/tasks/{id}/priority → 队列重排
└── 状态机: queued → running → done / cancelled / failed
```

- 每个任务独立 `PipelineRunner` 实例（复用现有 runner/取消/进度回调）；
- 批量任务（F3）内部的多 media 仍由单任务 runner 按现有 `execution_order` 处理，不入队；
- 与 CLI 任务互斥：Web 模式与 CLI 任务不同时运行（单进程内天然互斥）；
- 复用现有 `memory_strategy` 与引擎缓存。

### 4.7 依赖引入

- vcpkg 新增 `drogon`（含 `jsoncpp`、`brotli` 等传递依赖）；
- 静态资源路径从 `assets/web` 读取（支持相对 `--app-config` 与可执行文件目录两种解析，与现有资源加载策略一致）。

## 5. 前端架构设计

### 5.1 工程结构与页面

```
web/src/
├── pages/
│   ├── TaskListPage        # 任务列表 + 队列位置 + 优先级操作（WS 实时刷新）
│   ├── TaskCreatePage      # 任务配置表单（多素材上传、处理器、参数、人脸选择）
│   └── TaskDetailPage      # 进度条 + 前后对比 + 结果预览
├── components/
│   ├── FileUploader        # 多素材上传（F3：批量选择/多文件）
│   ├── MediaPreview        # 图片/视频预览（F1：video Range 播放）
│   ├── CompareSlider       # 前后对比滑块（F2：左右对比原图/结果）
│   ├── FrameExtractor      # 视频帧选取（F5：<video> 截帧上传）
│   ├── FaceSelector        # 人脸选择（F6：检测框叠加预览 + 点选 + reference 上传）
│   ├── ProcessorSelector   # 处理器选择（数据来自 /api/processors）
│   ├── ParamForm           # 动态参数表单（由参数元数据驱动）
│   └── ProgressView        # 进度条/FPS 展示（WS）
├── api/                    # REST client + WS client（自动重连）
└── types/                  # 与 API 对齐的 TS 类型
```

### 5.3 批量与队列交互（F3/F4）

- 任务创建页支持多选源/目标素材（一次上传，一次提交）；
- 任务列表页展示队列位置，提供"置顶/提升优先级"操作；
- 批量任务整体作为一个任务条目（单进度，内部按 media 顺序推进）。

### 5.2 开发与生产

- 开发：`vite dev`（`web/` 内）→ 代理 `/api`、`/ws` 到本地 C++ 服务（`--web-port 8000`）；
- 集成调试：`vite build` → `./ffc --web --web-root web/dist` 直接托管产物，免拷贝；
- 生产：`vite build` → 由 `build.py --action web` 同步至 `assets/web/` → Drogon 默认托管。

### 5.3 上传策略

- 素材上传：先落 C++ 服务临时目录（`temp_directory`，复用现有配置），任务提交引用路径；
- 视频/大图支持分片上传（第一阶段可简化为单次 POST）。

## 6. 构建集成

1. `build.py --action web`：`cd web && npm ci && npm run build` → 产物同步至 `assets/web/`；
2. CMake install：`install(DIRECTORY assets/web DESTINATION .)`（若存在）；
3. `--action build` 检测 `assets/web/` 缺失时仅警告（CLI 不受影响）；
4. CI（release.yml）：Release 构建前先执行 `--action web`；
5. 包产物体积预估：前端产物 < 1MB（gzip），影响可忽略。

## 7. 测试策略

| 层级 | 内容 |
| :--- | :--- |
| 单元测试 | `app.web` 的 REST handler 逻辑（Mock HTTP 客户端）、WebSocket 消息序列化 |
| 集成测试 | 启动真实 Drogon server + 测试客户端打 API（任务提交→进度→结果闭环） |
| e2e | `web_api` 场景：CLI 启动 `--web` → HTTP 提交任务 → WS 收进度 → 校验结果文件 |
| 前端测试 | Vitest 组件测试 + Playwright 冒烟（可选，第二阶段） |

## 8. 文档计划

- `docs/user/{zh,en}/web_guide.md`：Web 界面使用指南（新增，登记 index.md）；
- `docs/user/{zh,en}/cli_reference.md`：增补 `--web` 系列参数；
- `docs/dev/{zh,en}/architecture/web_ui_design.md`：本设计文档（英文版随实现阶段同步）；
- 修订历史与文档控制信息按项目规范维护。

## 9. 风险与缓解

| 风险 | 可能性 | 影响 | 缓解 |
| :--- | :--- | :--- | :--- |
| Drogon 依赖重量（jsoncpp/brotli 等）增加构建时间与体积 | 中 | 构建/体积 | 接受（功能收益大于成本）；备选 uWebSockets |
| 任务长时间运行期间 WS 断连 | 中 | 进度丢失 | 历史进度查询 API 补偿 + 前端自动重连 |
| 多任务排队/优先级导致 GPU 长时间占用 | 中 | 公平性/显存 | 单任务并发 + 优先级重排；显存预算与并发上限（后续里程碑） |
| 上传大文件内存占用 | 低 | 服务不稳定 | 分片上传/流式落盘（第一阶段限制文件大小） |
| 视频 Range 播放兼容性 | 低 | 预览异常 | Drogon 静态服务 Range 支持 + 前端降级（截帧预览） |
| 前端截帧精度（F5）受浏览器解码限制 | 低 | 帧不精确 | 阶段二提供服务端 ffmpeg 抽帧 API 兜底 |

## 10. 后续里程碑（实施计划另行制定）

1. **M1 骨架**：vcpkg 引入 Drogon、`app.web` 模块（静态托管 `--web-root` + /api/health）、CLI `--web` 入口、前端空壳工程（Vite+React+TS）、build.py --action web； ✅ **已完成**（2026-08-14，分支 feature/plan-web-ui-m1）
2. **M2 任务闭环 + 基础预览**：REST 任务提交/查询/取消、WebSocket 进度推送、任务创建/详情页、MediaPreview（F1）、CompareSlider（F2）、`/media/` 文件访问；
3. **M3 批量 + 队列**：TaskScheduler 队列与优先级（F4）、批量素材上传与提交（F3）、队列 UI 与优先级操作；
4. **M4 视频帧 + 人脸选择**：FrameExtractor（F5 前端截帧）、FaceSelector 与 `/api/faces` 检测标注（F6）、参考人脸上传、e2e 测试、用户文档；
5. **M5 打磨**：大文件分片上传、服务端抽帧 API（F5-B）、并发控制、国际化（i18n）、Playwright 测试。

> 设计评审通过后，由 writing-plans 流程生成详细实施计划（含分支、TDD 任务分解）。
