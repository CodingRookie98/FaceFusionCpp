# Web UI M4（视频帧截取 + 人脸选择与标注）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-M4-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-17

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | 依据设计规格 V0.2.0 创建并完成 M4 实施计划（F5 视频截帧 + F6 人脸检测标注与选择）。 |

---

**Goal:** 实现视频帧截取（FrameExtractor 前端截帧上传）、人脸检测标注 API（`/api/faces`）、人脸选择组件（FaceSelector）、参考人脸选择与任务配置联动、e2e 测试及用户文档。

**Architecture:** 
1. 后端：`WebServerDeps` 新增 `FaceDetectorFunc`，注册 `GET`/`POST /api/faces` 端点，支持图像路径的人脸检测、置信度与关键点标注 JSON 输出；
2. 前端：新增 `FrameExtractor`（`<video>` 截帧至 Canvas 并调用 `/api/upload`）与 `FaceSelector`（图片人脸检测框叠加渲染与点选），升级 `TaskCreatePage` 支持人脸选择模式（`many`/`one`/`reference`）与参考人脸配置；
3. 测试与文档：扩展 `web_api_test.cpp`、`web_api_test.py`、更新用户指南与架构设计文档。

**Tech Stack:** Drogon、nlohmann_json、OpenCV、React 19、TypeScript、HTML5 Canvas/Video。

---

## Global Constraints

- `/api/faces` 支持 `POST` 与 `GET`：
  - `POST /api/faces` 接收 JSON `{"image_path": "..."}` 或 `GET /api/faces?image=...`；
  - 路径安全性校验：拒绝 `..` 路径遍历、文件必须存在且为有效图像；
  - 返回统一人脸框（x, y, width, height）、分数、关键点、性别/年龄（若可用）；
- 解耦与可测试性：`WebServerDeps` 注入人脸检测函数指针/对象，测试时注入 Fake Detector，生产时使用 `domain::face::analyser::FaceAnalyser`；
- 前端截帧（F5 阶段一）：纯前端利用 `<video>` + `<canvas>` 导出 Blob 并复用 `/api/upload` 上传，获得服务端路径填入表单；
- 向后兼容：不破坏现有任务提交与 WebSocket 协议。

---

### Task 1: 服务端人脸检测类型与 `/api/faces` REST 端点 (TDD)

**Files:**
- Modify: `src/app/web/task_types.ixx`（定义 `DetectedFace`、`FaceDetectionResult`）
- Modify: `src/app/web/web_server.ixx`（`WebServerDeps` 增加 `detect_faces` 钩子）
- Modify: `src/app/web/web_server.cpp`（注册 `/api/faces` 路由与默认实现）
- Test: `tests/integration/app/web_api_test.cpp`（增补 `/api/faces` 测试）

**API 契约:**
| 方法 | 路径 | 请求 | 响应 |
| :--- | :--- | :--- | :--- |
| POST / GET | /api/faces | `{"image_path":"..."}` 或 `?image=...` | `{"image":"...","faces":[{"index":0,"box":{"x":10,"y":20,"width":80,"height":90},"score":0.95,"gender":"male","age_range":[20,30],"kps":[{"x":30,"y":40},...]}]}` |

**TDD 用例:**
- 正常图像人脸检测返回人脸列表及坐标
- 缺失路径或空请求返回 400
- 文件不存在返回 404
- 非法路径（如含有 `../`）拒绝 400
- 注入 mock detector 验证与默认兜底

---

### Task 2: 前端 API Client 与类型定义

**Files:**
- Modify: `web/src/api/types.ts`（增加 `FaceBox`, `DetectedFace`, `DetectFacesResponse`, `FaceSelectorMode`）
- Modify: `web/src/api/client.ts`（增加 `detectFaces(imagePath)` 方法）

---

### Task 3: 前端组件与页面交互 (FaceSelector + FrameExtractor)

**Files:**
- Create: `web/src/components/FaceSelector.tsx`（展示图片与人脸框高亮、点击点选人脸、展示序号与置信度）
- Create: `web/src/components/FrameExtractor.tsx`（加载视频、播放进度滑块选帧、截帧并上传为素材）
- Modify: `web/src/pages/TaskCreatePage.tsx`（整合视频截帧、人脸检测标注与参考人脸配置）
- Modify: `web/src/index.css`（人脸框标定与视频截帧样式）

**验证:**
- `npm run build`（Vite 编译与类型检查通过）
- 界面流畅加载检测人脸并高亮选中框

---

### Task 4: e2e 集成测试与文档完善

**Files:**
- Modify: `tests/e2e/scripts/web_api_test.py`（增加 `/api/faces` 与参考人脸提交验证）
- Modify: `docs/user/zh/web_guide.md` & `docs/user/en/web_guide.md`（更新人脸选择与视频截帧使用指南）
- Modify: `docs/dev/zh/architecture/web_ui_design.md`（标记 M4 里程碑为已完成）

---

## M4 验收标准

- [x] `/api/faces` 单元/集成测试全绿（含错误处理与边界测试）
- [x] 前端 TypeScript 编译通过，`build.py --action web` 构建成功
- [x] e2e 测试脚本全绿
- [x] 用户指南与架构设计文档同步更新
