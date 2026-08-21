# Web UI V2 Studio（沉浸式工作台与多实例管线）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-V2-2026
> - **当前版本 (Version)**: V1.3.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-21

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.3.0** | 2026-08-21 | AI Agent | 王辉 | 增补阶段 9：素材预览生命周期持久化（解决页面刷新后 Blob URL 失效问题、智能回落 /api/preview、localStorage 清洗）与页面刷新后人脸自动感知恢复。 |
| **V1.2.0** | 2026-08-21 | AI Agent | 王辉 | 增补阶段 8：实现“添加到任务队列”CTA、左栏三 Tab（源素材/目标素材/任务队列）、任务卡片优先级升降级与取消、中间画布源素材/目标素材/任务全态联动预览。 |
| **V1.1.0** | 2026-08-21 | AI Agent | 王辉 | 增补阶段 6 与阶段 7：实现多文件并发与拖拽上传、人脸多选/反选/全选/清空、处理结果独立预览与下载、C++ Web 全链路结构化日志及 Playwright E2E 自动化端到端测试。 |
| **V1.0.0** | 2026-08-20 | AI Agent | 王辉 | 依据 Web UI 设计规格 V0.4.0 创建 Web UI V2 Studio 重构实施计划（3栏工作台、多实例动态管线、WYSIWYG 人脸映射、卷帘/放大镜多模态对比、HUD 调度与历史持久化）。 |

---

## 1. 目标与范围 (Goal & Scope)

**目标:**
将现有分散的分页式 Web 界面彻底重塑为一体化 **Deep Studio Dark 沉浸式创作工作台**，实现从素材输入、交互人脸点选绑定、动态多处理器实例管线配置、到实时任务调度及多模态前后对比的完整闭环。

**核心交付物:**
1. **C++ 后端增强**: `/api/tasks` 扩展支持 `pipeline_steps` 结构化多步骤数组解析，支持同类型 Processor 多实例挂载与参数独立绑定；
2. **前端工程升级**: 引入 Tailwind CSS + Lucide Icons，构建 Deep Studio Dark 专业暗色设计系统；
3. **Studio 3 栏工作台**:
   - **左栏 (`AssetPool`)**: 源人脸图库、目标媒体池、内置测试样例一键载入、视频时间轴截帧；
   - **中栏 (`ViewportCanvas`)**: 交互主画布、WYSIWYG 人脸框绘制与点选绑定、卷帘滑动对比 (Split-Slider)、局部细节放大镜 (Loupe)、视频同频播放；
   - **右栏 (`PipelineEditor`)**: 多实例卡片堆叠、动态增删与排序、预设场景快速切换、模型与浮点参数精准调节；
   - **底栏 (`TelemetryHUD`)**: 常驻实时进度条、FPS/帧率遥测、任务队列优先级调度、LocalStorage 历史任务回放与一键复用参数。
4. **自动化测试与验证**: 前端 Vitest 测试、C++ Web 接口单元与集成测试、e2e 回归测试。

---

## 2. 技术规格与约束 (Constraints & API Contract)

1. **向后兼容**:
   - `/api/tasks` 请求体若包含 `pipeline_steps`（数组），优先使用其构建 `TaskConfig.pipeline`；若不包含，平滑回退到旧版 `processors` + `processor_params` 解析，保证旧脚本与客户端完全兼容。
2. **单二进制交付**:
   - 前端构建产物（`web/dist`）经 `python build.py --action web` 编译打包至 `assets/web/`，依然随 `ffc --web` 单二进制分发。
3. **TDD 流程**:
   - C++ 侧 `web_server.cpp` 的 `pipeline_steps` 解析与边界校验严格遵循 TDD 先写单元测试再实现；
   - 前端核心工具函数与交互组件编写对应 Vitest 单元测试。

---

## 3. 分阶段任务清单 (Phased Tasks)

### 阶段 1: C++ Web 后端 API 协议升级 (TDD)
- **目标**: `/api/tasks` 支持接收并解析 `pipeline_steps: Array<{ step, name, enabled, params }>`
- **文件**:
  - `src/app/web/web_server.cpp`: 扩充 `parse_create_task_request`
  - `src/app/web/task_types.ixx`: 补充 TS 对齐数据定义（如需）
  - `tests/unit/app/web_server_test.cpp`: 编写 `pipeline_steps` 正常解析、多 swapper 实例、无效 step 类型、缺省 params 等用例
- **验收标准**: 单元测试全绿，通过 Mock 请求验证多实例管线正确构造 `config::PipelineStep` 序列。

### 阶段 2: 前端工程基础设施与设计系统搭建
- **目标**: 引入 Tailwind CSS、Lucide 图标库、建立 Deep Studio Dark 调色板与原子 UI 组件库
- **文件**:
  - `web/package.json`: 添加 `lucide-react`, `tailwindcss`, `@tailwindcss/vite` 或相应 PostCSS 工具链
  - `web/vite.config.ts`: 配置 Tailwind 插件与路径别名
  - `web/src/index.css`: 配置 Studio Dark 主题变量、Scrollbar 样式与动画
  - `web/src/components/ui/`: Button, Slider, Switch, Badge, Modal, Tooltip, Dropdown
- **验收标准**: `cd web && npm run build` 编译通过，原子组件渲染正常。

### 阶段 3: 工作台核心组件群构建
- **目标**: 实现左侧素材池、右侧动态管线编排器、中间主视口画布与底栏 HUD
- **文件**:
  - `web/src/api/types.ts` & `client.ts`: 扩展 `PipelineStepConfig`, `CreateTaskRequest`
  - `web/src/components/media/AssetPool.tsx`: 源图/目标素材上传、缩略图列表、内置 sample 样例
  - `web/src/components/media/VideoFrameGrabber.tsx`: 视频选帧工具
  - `web/src/components/pipeline/PipelineEditor.tsx`: 管线编排容器
  - `web/src/components/pipeline/PipelineStepCard.tsx`: 单个 Processor 卡片（支持多个 face_swapper、改名、拖拽/上移下移、滑块）
  - `web/src/components/pipeline/PresetSelector.tsx`: 4 种官方预设场景一键切换
  - `web/src/components/canvas/ViewportCanvas.tsx`: 主画布视口
  - `web/src/components/canvas/FaceOverlay.tsx`: 人脸框高亮、置信度/性别标签、点选绑定 Reference
  - `web/src/components/canvas/SplitSlider.tsx`: 卷帘对比滑动组件
  - `web/src/components/canvas/DetailLoupe.tsx`: 局部放大镜
  - `web/src/components/hud/TelemetryHUD.tsx`: 实时进度、FPS、排队调度、历史回放
- **验收标准**: 工作台各区域协同工作，人脸点选、预设切换、管线增删、卷帘对比交互丝滑。

### 阶段 4: 工作台整合、状态流转与本地持久化
- **目标**: 将 Studio Workbench 设为主界面，集成 LocalStorage 历史任务管理与参数一键恢复
- **文件**:
  - `web/src/App.tsx`: 挂载 Studio Workbench，替换原有分页布局
  - `web/src/store/studioState.ts`: 工作台状态管理（当前选定素材、管线配置、对比结果、历史任务）
  - `web/src/components/hud/HistoryModal.tsx`: 历史任务结果抽屉与参数复用
- **验收标准**: 页面刷新后自动保持最近使用的素材与管线预设，历史任务一键载入对比。

### 阶段 5: 全链路质量验收与文档更新
- **目标**: 运行所有单元测试、端到端测试，同步用户文档与索引
- **文件**:
  - `tests/e2e/scripts/web_api_test.py`: 增加 `pipeline_steps` API 自动化验证
  - `docs/user/zh/web_guide.md` & `docs/user/en/web_guide.md`: 更新 Studio 界面操作指南与多实例管线教程
  - `docs/index.md`: 更新文档索引与版本号
- **验收标准**: `python build.py --action test --test-label unit` 与 `npm run test` 全绿，`build.py --action web` 打包成功。

### 阶段 6: 多文件并发、人脸多选反选与智能结果匹配增强
- **目标**: 素材池支持多文件并发上传与拖拽放置；画布支持人脸多选、反选、全选/清空；结果支持全图预览、一键下载与智能结果匹配
- **文件**:
  - `web/src/components/media/AssetPool.tsx`: 多文件并发上传与拖拽导入
  - `web/src/components/canvas/FaceOverlay.tsx`: 多脸红框高亮、选中/未选标签提示与点击反选切换
  - `web/src/components/canvas/ViewportCanvas.tsx`: 全选/清空控制栏、结果独立预览模式与下载按钮
  - `web/src/store/studioState.ts`: 多人脸状态管理与步骤参数智能联动
- **验收标准**: Vitest 35 项测试全绿，画布多脸交互丝滑，结果匹配 100% 准确。

### 阶段 7: C++ Web 模块日志系统化与 Playwright 全栈 E2E 自动化测试
- **目标**: C++ Web 服务全生命周期结构化日志，Playwright E2E 直连真实后端全链路自动化测试
- **文件**:
  - `src/app/web/web_server.cpp`: 路由日志、MIME 类型推导、CORS 支持
  - `src/app/web/task_manager.cpp`: 任务生命周期日志、精准结果收集
  - `src/app/web/pipeline_executor.cpp`: 流水线执行与取消日志
  - `web/e2e/studio.spec.ts` & `web/e2e/studio.live.spec.ts`: Playwright E2E 测试套件
- **验收标准**: Playwright 11 项端到端测试全绿，C++ 262 项单元测试全绿。

### 阶段 8: 任务队列面板、队列卡片优先级升降级与全态画布联动预览
- **目标**: 
  1. 将右侧管线“启动渲染任务”按钮改为“添加到任务队列”；
  2. 左侧面板重构为 3 个一级 Tab（`源素材` / `目标素材` / `任务队列 (N)`）；
  3. 任务队列面板渲染排队中、运行中、已完成、失败任务卡片，提供【⬆️ 提升优先级】、【⬇️ 降低优先级】、【✕ 取消任务】；
  4. 中间主视口画布实现全态感知联动预览（源素材预览态、目标素材人脸点选态、任务队列结果/进度态）。
- **文件**:
  - `web/src/components/media/AssetPool.tsx`: 整合“任务队列”Tab 与队列列表渲染
  - `web/src/components/pipeline/PipelineEditor.tsx`: 调整 CTA 文本为“添加到任务队列”
  - `web/src/components/canvas/ViewportCanvas.tsx`: 增加源素材预览与任务队列多态预览适配
  - `web/src/store/studioState.ts`: 增加 `activePreviewItem` 全态预览状态、任务升降级与取消 API 绑定
  - `web/src/components/media/AssetPool.test.tsx` & `web/src/components/canvas/ViewportCanvas.test.tsx`: 单元测试
  - `web/e2e/studio.live.spec.ts`: Playwright 全流程端到端测试
- **验收标准**: Vitest 与 Playwright 测试全绿，源素材/目标素材/任务队列点击即时联动中间画布预览，任务优先级升降级与取消操作即时响应。

### 阶段 9: 素材预览生命周期持久化与自动人脸感知恢复
- **目标**: 彻底消除页面刷新后上传素材预览空白的问题，实现 Blob 临时链接与后端 `/api/preview` 静态源的平滑回落，并在页面刷新挂载后自动感知恢复目标素材的人脸检测框标注。
- **任务清单**:
  1. **素材预览智能回落**: 在 `getMediaPreviewUrl` 中优先检查本地有效 `file` 对象，当处于刷新后状态或 `thumbnailUrl` 带有失效 `blob:` 标识时，自动回退至后端 `/api/preview?path=${encodeURIComponent(item.path)}`。
  2. **状态清洗与持久化**: 在 `sanitizeSources`、`sanitizeTargets` 以及写入 `localStorage` 时，清洗掉临时 `blob:` 链接，确保存储的数据具备跨页面刷新可用性。
  3. **人脸检测自动感知恢复**: 在 `useStudioStore` 初始加载/目标素材挂载时，若当前存在选中的目标素材且人脸列表为空，自动向后端发起静默人脸检测，无需用户二次操作。
  4. **单元测试与回归**: 编写针对页面刷新后持久化恢复、Blob 链接失效降级与自动人脸恢复的单元测试用例。
- **涉及文件**:
  - `web/src/store/studioState.ts`: `getMediaPreviewUrl` 智能降级、`sanitizeSources`/`sanitizeTargets` 清洗、自动感知人脸检测触发
  - `web/src/store/studioState.test.ts`: 编写持久化与回退机制单元测试
  - `web/src/components/canvas/ViewportCanvas.tsx`: 验证刷新后画布渲染
- **验收标准**: 页面刷新后，上传的源素材/目标素材缩略图与画布视图均能正常从 `/api/preview` 加载显示，当前选中的目标素材自动恢复人脸框检测，Vitest 与 E2E 测试全部通过。

---

## 4. 实施状态跟踪

| 阶段 | 状态 | 负责人 | 完成日期 |
| :--- | :--- | :--- | :--- |
| 阶段 1: C++ 后端 API 升级 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 2: 前端工程与设计系统 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 3: 工作台核心组件群 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 4: 工作台整合与持久化 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 5: 全链路质量验收 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 6: 多文件与人脸多选增强 | 已完成 | AI Agent | 2026-08-21 |
| 阶段 7: 全链路日志与 E2E 测试 | 已完成 | AI Agent | 2026-08-21 |
| 阶段 8: 任务队列与全态画布预览 | 已完成 | AI Agent | 2026-08-21 |
| 阶段 9: 素材预览生命周期与人脸感知恢复 | 进行中 | AI Agent | 2026-08-21 |
