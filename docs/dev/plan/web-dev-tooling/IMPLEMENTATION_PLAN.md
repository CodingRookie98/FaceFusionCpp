# Web 开发工具链改进（端口可配 + WS 自动重连 + build.py dev）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBDEVTOOLING-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-17

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-08-17 | AI Agent | 王辉 | 三个任务全部完成并验证：Task 1 vite 端口可配（FFC_WEB_PORT/FFC_WEB_HOST）、Task 2 WS 自动重连（vitest 9/9 + tsc + build 通过）、Task 3 build.py --action dev（端口冲突预检/FFC_WEB_PORT 联动/退出清理，实测通过）；文档已同步（build.md/setup.md zh+en/web_ui_design.md）。 |
| **V1.0.0** | 2026-08-17 | AI Agent | 王辉 | 依据 Web UI 开发体验排查结论创建：修复端口硬编码、WS 无自动重连、无一键启动三大痛点。 |

---

**Goal:** 改善 `ffc --web` + Vite 的开发调试体验：代理目标端口可配置；前端 WebSocket 断线自动重连（指数退避）；`build.py --action dev` 一键启动后端与前端。

**背景（实证排查结论）:**
1. `web/vite.config.ts` 将代理目标硬编码为 `127.0.0.1:8000`，C++ 端改端口即断连；
2. `web/src/api/client.ts` 的 `subscribeProgress` 无 `onclose`/`onerror` 重连，后端重编译重启后前端进度卡死，需手动刷新；
3. 后端（须在 `build/bin/<preset>` 下运行）与前端（`npm run dev`）需手工双进程启动，且旧实例占用 8000 端口时新实例静默崩溃、前端仍连旧实例——无一键入口、无冲突提示；
4. 环境坑：`NODE_ENV=production` 会使 `npm ci` 跳过 devDependencies（已实证）。

**Architecture:** 本次为**前端/脚本层工具链改动**，不触碰 C++ 架构（不涉及前后端进程分离，代理链路已实证可用）。

**Tech Stack:** Vite 6、TypeScript、Vitest（新增，待批准）、Python 3（build.py）。

---

## Global Constraints

- 不改变生产构建链路：`build.py --action build` / `--action web` 行为保持不变；
- 不改变 REST/WS 协议与 C++ 侧任何代码；
- WS 重连依赖服务端已有的断线补发能力（`handleNewConnection` 会推送当前 status + progress，web_server.cpp:306-309）；
- 端口环境变量命名：`FFC_WEB_PORT`（后端端口）、可选 `FFC_WEB_HOST`；
- `NODE_ENV` 防御：dev 启动路径显式 unset，避免 npm 漏装 devDependencies；
- 简单至上：不做超出三大痛点之外的重构。

---

### Task 1: vite 代理端口可配置（配置改动 + 手动验证）

**Files:**
- Modify: `web/vite.config.ts`

**改动:**
- 代理目标从 `process.env.FFC_WEB_PORT ?? '8000'`（及 `FFC_WEB_HOST ?? '127.0.0.1'`）读取；
- `/ws` 代理目标同步使用同一端口。

**TDD 说明:** 纯配置声明，无业务逻辑，豁免单测；以手动验证计划覆盖。

**验证计划（手动）:**
1. `FFC_WEB_PORT=9000` 起 Vite dev → `curl http://127.0.0.1:5173/api/health` 应返回 C++ 服务 JSON（后端以 `--web-port 9000` 启动）；
2. 默认（未设变量）仍为 8000，原行为不变。

---

### Task 2: WebSocket 自动重连（TDD，引入 Vitest）

**Files:**
- Modify: `web/src/api/client.ts`
- Add: `web/vitest.config.ts`、`web/src/api/reconnect.test.ts`（或 `__tests__/`）
- Modify: `web/package.json`（新增 `vitest` devDependency + `"test": "vitest run"` script）

**改动:**
- 抽出纯函数 `computeReconnectDelay(attempt, baseMs=1000, maxMs=30000)`：`min(baseMs * 2^attempt, maxMs)`，`attempt` 从 0 开始；
- 改造 `subscribeProgress`：`onclose`/`onerror` 触发重连，指数退避 + 上限（1s→2s→4s…→30s）；连接成功后 `attempt` 归零；`unsubscribe` 置 `closed` 标志阻止后续重连（防泄漏）；
- 重连成功后依赖服务端补发机制恢复状态，无需额外拉取。

**TDD 用例（vitest）:**
- `computeReconnectDelay`：退避递增、封顶 30s、attempt 0 返回 1s；
- `subscribeProgress` 重连行为（mock 全局 `WebSocket`）：
  - 断线后自动重连且重连延迟符合退避；
  - `unsubscribe` 后不再重连；
  - 连接成功后下一次退避从 1s 重新开始。

**决策点（需用户批准）:** 引入 `vitest` 作为前端测试基础设施（devDependency，不影响生产产物）。依据：AGENTS.md TDD 强制 + 设计文档 7 已规划"前端测试 Vitest（第二阶段）"，本次补上设施并落地首个用例。备选方案：按"浏览器强依赖代码"豁免走手动测试计划。

---

### Task 3: `build.py --action dev` 一键启动（脚本 + 手动验证）

**Files:**
- Modify: `build.py`（新增 `dev` action 与参数 `--web-port`）

**改动:**
- `python build.py --action dev [--web-port 8000]`：
  1. 确保已构建（未构建则先 `--action build`）；
  2. **端口冲突预检**：检测目标端口已被监听时打印明确错误（提示"可能已有旧 ffc --web 实例占用，请先清理"）并退出，避免静默连旧实例；
  3. 在 `build/bin/<preset>` 下后台启动 `ffc --web --web-port <port>`；
  4. 等待 `/api/health` 就绪后，在 `web/` 下前台启动 `vite dev`（`unset NODE_ENV` 防御）；
  5. 进程退出/中断时自动清理子进程（`finally`/信号处理），不留残留。

**TDD 说明:** 进程编排 + 外部依赖（进程、端口、npm），按 AGENTS.md 强外部依赖代码走**手动测试计划**，不引入 pytest（避免过度工程）。

**验证计划（手动）:**
1. `python build.py --action dev` → 自动起后端 + Vite，`curl :5173/api/health` 通、WS 握手 101；
2. 端口占用场景：先手动起一个 `ffc --web` 占用 8000，再 `--action dev` → 应打印冲突错误并退出；
3. `Ctrl-C` 中断后 `ss -tlnp | grep 8000` 应无残留 ffc 进程。

---

## 阶段五文档计划（合并后）

- Modify: `docs/dev/{zh,en}/guides/setup.md` — 增补 `--action dev` 一键启动与 `FFC_WEB_PORT` 环境变量说明；
- Modify: `docs/dev/zh/architecture/web_ui_design.md` — 仅更新 5.2 开发与生产章节（dev 一键启动、端口可配、WS 重连）；**不**顺手改动本次已识别的失实内容（priority 语义等留待 M5 统一处理）；
- Modify: `docs/build.md` — 若存在，增补 dev action；
- 不涉及 C++ 模块/CLI 参数变更，`cli_reference.md` 无需改动。

---

## 分支与提交

- 分支：`feature/plan-web-dev-tooling`（基于 `dev`，master 不领先 dev 无需预合并）；
- 提交顺序：Task1 → Task2（含 vitest 引入）→ Task3 → 文档，每任务独立 commit；
- 测试分层（AGENTS.md）：前端配置/逻辑改动无 C++ 侧影响，跑 `--action web` 前端构建验证 + vitest；C++ 侧零改动，无需跑 C++ 单测（提交说明注明）。
