# Web UI 设计文档（web_ui_design.md）M1-M4 完成度对抗性复核报告

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-EVAL-WEBUI-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 待审批 (Pending Approval)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-17

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | 基于第一性原理与对抗性审查，对 web_ui_design.md 声称的 M1-M4 完成状态进行证据复核。 |

---

## 1. 复核范围与方法

**复核对象**: [web_ui_design.md](../architecture/web_ui_design.md)（V0.3.0，声称 M1-M4 已完成）

**复核方法**:
1. **第一性原理验收标准**: 以"用户真实运行 `ffc --web` 能获得的能力"为准绳，而非"测试是否变绿"；
2. **对抗性审查**: 逐项对照文档声称（里程碑、REST API 草案 4.4、组件清单 5.1、风险缓解 9、文档计划 8）与实际代码、测试、构建产物，找出"声称 ≠ 实现"；
3. **证据优先**: 每个发现均给出代码位置或命令输出，可复现。

## 2. 验证证据（已实际执行）

| 验证项 | 结果 | 证据 |
| :--- | :--- | :--- |
| git 分支合并 | M1-M4 分支均已合并 | ba000ae / 0c3b3ca / 5b039b4 / 25a1388 |
| 构建 | `build.py --action build` 43/43 通过 | exit 0，ffc 生成 |
| 单元测试 | web_task_manager_test 10/10、web_server_health_test 1/1 | gtest PASSED |
| 集成测试 | web_server_tests 1/1、web_ws_tests 2/2、web_api_tests 8/8 | 连续两轮全绿；首轮偶发 1 失败（见 P2-6） |
| e2e | `web_api_test.py` 全绿 | health/静态/提交/上传/priority/faces/reference |
| 前端构建 | `build.py --action web` 成功 | 需 `unset NODE_ENV`（见 P2-7） |
| CLI | `--web` 系列参数 + excludes 互斥 | app_cli.cpp:179-187, 180-182 |
| 安全防护 | 路径穿越拦截、result 白名单 | web_server.cpp:505-512, 469-475, 687-711 |

## 3. 发现的问题（按严重度排序）

### 🔴 P0 — 功能不可用（测试假绿掩盖）

**P0-1. `/api/faces` 人脸检测在生产环境为空壳，M4 核心功能对真实用户不可用**
- 现象: 真实 `ffc --web` 下 `/api/faces` 永远返回 `faces: []`，前端 FaceSelector 永远显示"0 张人脸"。
- 根因: `WebServerDeps::detect_faces`（web_server.ixx:43，默认 `nullptr`）**从未在生产注入**——`run_web_mode`（app_cli.cpp:291-308）只传 tasks+app_config；web_server.cpp:485 的 `if (deps.detect_faces)` 恒为 false。
- 为何测试仍绿: 集成测试注入了 mock detector（web_api_test.cpp:72）；e2e 仅断言 `"faces" in body`（空数组也通过）。
- 结论: 文档声称 M4"实现 /api/faces 检测标注"**不成立**。

### 🟠 P1 — 文档-实现不一致 / 已登记风险未缓解

**P1-1. WS 断线补偿双缺失**
- 文档 4.4 的 `GET /api/tasks/{id}/progress`（历史进度查询）未实现；
- `subscribeProgress`（client.ts:53-70）无 onclose/onerror 自动重连；
- 文档 9 承诺的"历史进度查询 API 补偿 + 前端自动重连"均未落地，风险未缓解。

**P1-2. `/api/processors` 未实现，处理器/参数无法在 UI 配置**
- 文档 4.4 列明 `GET /api/processors`；5.1 声称 ProcessorSelector/ParamForm"数据来自 /api/processors、参数元数据驱动"；
- 实际前端硬编码处理器列表与 face_swapper 参数（TaskCreatePage.tsx:12-17, 82-94），其他处理器参数无法配置。

**P1-3. F1 视频预览未达成（HTTP Range 缺失）**
- `serve_file`（web_server.cpp:227-245）全量读入内存、无 Range/206/Content-Range；
- `/media` 的 source/target/result 全部强制 `CT_IMAGE_PNG`（web_server.cpp:660-718），视频文件也会按 PNG 返回；
- 前端仅 img 对比（TaskDetailPage.tsx:98-112），无 video 播放；文档 F1"Drogon 原生 Range 支持"未兑现。

**P1-4. priority 语义文档写反**
- 文档 4.6 写"数值小优先；默认 0"，实现为**数值大优先**（task_types.ixx:38、task_manager.cpp:229-238 "priority desc"），前端 UI 亦标注"数值大优先"。行为自洽但文档描述相反。

### 🟡 P2 — 质量 / 过程缺口

**P2-1. web_api_tests 偶发 flaky**
- 首次全量跑 `PriorityEndpointWorks` 失败（5006ms 超时），随后 3 轮全过；
- 根因: 全局共享 TaskManager（web_api_test.cpp:62）+ 固定等待窗口，测试隔离弱，机器负载高时可能假失败。

**P2-2. `build.py --action web` 对 `NODE_ENV=production` 无防御**
- 实测: shell 带 `NODE_ENV=production` 时 `npm ci` 跳过 devDependencies → `tsc: not found` → exit 127；
- `unset NODE_ENV` 后成功。CI/生产 shell 易踩。

**P2-3. 英文版设计文档缺失**
- 文档 8 声称"英文版随实现阶段同步"，`docs/dev/en/architecture/` 下无 web_ui_design.md（仅有 design.md/layers.md）。

**P2-4. e2e"闭环"声称未完全兑现**
- 文档测试策略 e2e 声称"HTTP 提交任务 → WS 收进度 → 校验结果文件"；
- 实际 web_api_test.py 无 WS 客户端断言、无结果文件校验；且真实环境任务直接 failed（无模型资源），未走到 done。

**P2-5. 组件清单与实现不符**
- 文档 5.1 的 MediaPreview/CompareSlider/ProcessorSelector/ParamForm/ProgressView 未按清单拆分（功能内联于页面 + ProgressBar）；
- 文档测试策略声称的"REST handler 单元测试（Mock 客户端）、WS 消息序列化单测"实际未建（WS 在 integration 层用真实 socket）。

### ⚪ P3 — 观察

**P3-1. FaceSelector 参考人脸语义**
- 参考人脸 = 检测源图**整图路径**（handleFaceSelected 提交 detectImagePath），非人脸裁剪区域；无独立"参考人脸上传"能力（仅视频截帧 reference 分支）。

## 4. 结论

- **M1 / M2 / M3 基本完成**：交付物落盘、构建与测试有据（unit 11/11、integration 12/12、e2e 全绿）；
- **M4 不能宣称完成**：P0-1 使"人脸检测标注"在真实 `ffc --web` 下空转，且被测试假绿掩盖；
- **文档存在多处失实声称**：风险缓解承诺（P1-1）、组件/API 声称（P1-2, P2-5）、priority 语义（P1-4）、英文版同步（P2-3）、e2e 闭环（P2-4）。

## 5. 后续建议

1. **P0-1 应纳入 M5 打磨验收清单第一优先**：在 `run_web_mode` 注入真实人脸检测器（domain::face::analyser::FaceAnalyser → `DetectedFaceInfo`），并强化 e2e 断言（faces 非空）。
2. 其余 P1 缺口（WS 断线补偿、processors API、Range、priority 文档修正）随 M5 统一处理。
3. 文档同步修正（web_ui_design.md 失实章节）建议单独任务跟进。

> 注: 本报告为临时审批工具，审批并实施完成后按项目规范删除，审计结论沉淀于目标文档修订历史与 index.md。
