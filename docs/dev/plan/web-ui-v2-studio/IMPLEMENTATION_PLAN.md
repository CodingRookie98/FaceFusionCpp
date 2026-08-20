# Web UI V2 Studio（沉浸式工作台与多实例管线）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-V2-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
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

---

## 4. 实施状态跟踪

| 阶段 | 状态 | 负责人 | 完成日期 |
| :--- | :--- | :--- | :--- |
| 阶段 1: C++ 后端 API 升级 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 2: 前端工程与设计系统 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 3: 工作台核心组件群 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 4: 工作台整合与持久化 | 已完成 | AI Agent | 2026-08-20 |
| 阶段 5: 全链路质量验收 | 已完成 | AI Agent | 2026-08-20 |

