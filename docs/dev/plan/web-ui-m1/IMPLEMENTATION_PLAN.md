# Web UI M1（骨架）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-WEBUI-M1-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 进行中 (In Progress)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-14

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-14 | AI Agent | 王辉 | 依据设计规格 [web_ui_design.md](../../zh/architecture/web_ui_design.md) V0.2.0 创建 M1 实施计划。 |

---

**Goal:** 跑通 `ffc --web`：Drogon 服务启动（`/api/health` + 静态托管），前端空壳可访问，`build.py --action web` 打通构建链路。

**Architecture:** 新增 `app.web` 模块（app 层，与 `app.cli` 平级）封装 Drogon；CLI 新增 `--web` 系列参数启动 Web 模式；`web/` 前端空壳工程（Vite+React+TS）；前端产物经 `build.py --action web` 落入 `assets/web/`。

**Tech Stack:** Drogon (vcpkg)、CLI11（现有）、Vite 6 + React 19 + TypeScript、build.py（现有 Python 构建脚本）。

## Global Constraints

- C++20 模块化（`.ixx` 接口 + `.cpp` 实现），模块军规遵守现有 CMake 模式（参考 `src/app/cli/CMakeLists.txt`）
- 依赖方向：`app.web → services.pipeline / config_core / foundation_infrastructure / app_version`，禁止反向
- 二进制名为 `ffc`（重命名已完成）；`--web` 与 `-s/-t/-o/-c` 互斥（CLI11 excludes）
- 前端产物（`web/dist`、`assets/web`）不提交 git
- 运行程序工作目录必须为 `build/bin/<preset>`（项目规则）
- ⚠️ 本沙箱环境 vcpkg 目录只读：Task 1（vcpkg install drogon）需在用户本机执行或申请沙箱升级

---

### Task 1: vcpkg 引入 drogon 依赖

**Files:**
- Modify: `vcpkg.json`（dependencies 数组追加 `"drogon"`）

**Interfaces:**
- Produces: CMake 可用 `find_package(Drogon CONFIG REQUIRED)`，链接 `Drogon::Drogon`

- [ ] **Step 1: 修改 vcpkg.json**

在 `dependencies` 数组追加（保持字母序，放在 `dp-thread-pool` 之后）：
```json
"drogon",
```

- [ ] **Step 2: 安装依赖**

Run: `cmake --preset linux-debug -DBUILD_TESTING=OFF`（触发 vcpkg install）
Expected: vcpkg 安装 drogon 及传递依赖（trantor/jsoncpp/brotli/zlib）成功
⚠️ 沙箱环境若报 vcpkg 只读，需在用户本机执行 `python build.py --action configure` 或批准沙箱升级。

- [ ] **Step 3: 验证 find_package**

在 `src/app/web/CMakeLists.txt`（Task 2 创建）中：
```cmake
find_package(Drogon CONFIG REQUIRED)
```
Expected: configure 无 "Could not find Drogon" 错误。

- [ ] **Step 4: Commit**

```bash
git add vcpkg.json
git commit -m "build(vcpkg): 引入 drogon 依赖"
```

---

### Task 2: app_web 库——WebServer 模块（核心）

**Files:**
- Create: `src/app/web/web_server.ixx`（接口）
- Create: `src/app/web/web_server.cpp`（实现）
- Create: `src/app/web/CMakeLists.txt`
- Modify: `src/app/CMakeLists.txt`（add_subdirectory(web) + 主 target 链接 app_web）
- Test: `tests/integration/app/web_server_test.cpp`（GTest）

**Interfaces:**
- Produces:
```cpp
export module app.web.server;

export namespace app::web {

struct WebServerOptions {
    std::string host = "0.0.0.0";
    uint16_t port = 8000;
    std::string web_root = "assets/web";  // 前端静态资源根
};

/// 启动 Drogon HTTP 服务（阻塞，直到 stop 或进程退出）
/// 路由: GET /api/health → {"status":"ok","version":"..."}
/// 静态托管: / → web_root 下的 index.html 与静态资源
void run_server(const WebServerOptions& options);

/// 生成 /api/health 响应 JSON 文本（供测试与 handler 复用）
std::string health_json();

}
```

- [ ] **Step 1: 写失败测试**（tests/integration/app/web_server_test.cpp）

```cpp
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <drogon/drogon.h>
#include <string>
import app.web.server;
using namespace app::web;

namespace {
constexpr uint16_t kTestPort = 18080;

void StartServerInThread(const std::string& web_root) {
    std::thread([web_root]() {
        run_server({.host = "127.0.0.1", .port = kTestPort, .web_root = web_root});
    }).detach();
    std::this_thread::sleep_for(std::chrono::seconds(1)); // 等待服务就绪
}

std::string Get(const std::string& path) {
    auto client = drogon::HttpClient::newHttpClient("http://127.0.0.1:" + std::to_string(kTestPort));
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Get);
    req->setPath(path);
    auto resp = client->sendRequest(req);
    return resp ? std::string(resp->getBody()) : std::string();
}
}

TEST(WebServerTest, HealthEndpoint) {
    StartServerInThread("");
    auto body = Get("/api/health");
    EXPECT_NE(body.find(""status":"ok""), std::string::npos);
    EXPECT_NE(body.find(""version""), std::string::npos);
}

TEST(WebServerTest, StaticIndexServed) {
    // web_root 指向临时目录，内含 index.html
    // （fixture 中创建 tempdir + index.html）
    StartServerInThread(kTempWebRoot);
    auto body = Get("/");
    EXPECT_NE(body.find("FaceFusionCpp"), std::string::npos);
}
```

- [ ] **Step 2: 运行验证失败**

Run: `python build.py --action test --test-label integration --test-regex WebServerTest`
Expected: 编译失败（app.web.server 模块不存在）→ Red

- [ ] **Step 3: 最小实现**

`web_server.ixx`（接口如上）；`web_server.cpp` 核心实现：
```cpp
module;
#include <drogon/drogon.h>
#include <string>
module app.web.server;
import app.version;

namespace app::web {

std::string health_json() {
    return "{\"status\":\"ok\",\"version\":\"" + std::string(app::version::version()) + "\"}";
}

void run_server(const WebServerOptions& options) {
    auto& app = drogon::app();
    app.addListener(options.host, options.port);
    app.registerHandler("/api/health", [](const drogon::HttpRequestPtr&, auto&& cb) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setBody(health_json());
        cb(resp);
    });
    if (!options.web_root.empty() && std::filesystem::exists(options.web_root)) {
        app.setDocumentRoot(options.web_root);
        app.setStaticFileRoute("/");
    }
    app.run();
}

}
```
（index.html 测试的临时目录在 fixture 创建）

- [ ] **Step 4: 运行验证通过**

Run: 同上
Expected: 2 个测试 PASS（服务可启动、health 与静态页可达）

- [ ] **Step 5: CMake 接线**

`src/app/web/CMakeLists.txt`（仿 app_cli 模式）：
```cmake
add_library(app_web)
target_sources(app_web
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES web_server.ixx
    PRIVATE web_server.cpp
)
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(app_web PRIVATE app_version Drogon::Drogon)
```
`src/app/CMakeLists.txt`：`add_subdirectory(web)` + 主 target 链接 `app_web`。

- [ ] **Step 6: Commit**

```bash
git add src/app/web tests/integration/app/web_server_test.cpp src/app/CMakeLists.txt
git commit -m "feat(web): 新增 app.web WebServer 模块（health + 静态托管）"
```

---

### Task 3: CLI 集成 `--web` 参数

**Files:**
- Modify: `src/app/cli/app_cli.ixx`（run_web_mode 声明）
- Modify: `src/app/cli/app_cli.cpp`（参数注册 + 分发 + run_web_mode 实现）
- Modify: `src/app/cli/CMakeLists.txt`（链接 app_web）

**Interfaces:**
- Consumes: `app::web::run_server(WebServerOptions)`、`WebServerOptions{host,port,web_root}`
- Produces: CLI 参数 `--web`、`--web-port`（默认 8000）、`--web-host`（默认 0.0.0.0）、`--web-root`（默认 assets/web）

- [ ] **Step 1: 写失败测试**（tests/integration/app/cli_web_flags_test.cpp）

用 GTest 直接调用 `App::run` 不现实（静态/私有），改为验证 CLI11 解析层：
```cpp
// 通过构造 argc/argv 调用 app 解析（复用 app_cli 内部注册逻辑不可行时，
// 最低限度验证：--web 与 -s 互斥报错、--web-port 非法值报错）
```
（若 App::run 不可测，则以 Task 2 的集成测试 + Task 6 的 e2e 作为行为验证，本任务以代码审查 + 编译通过为验收）

- [ ] **Step 2: 实现参数与分发**

app_cli.cpp 中注册（仿现有全局选项）：
```cpp
bool web_mode = false;
uint16_t web_port = 8000;
std::string web_host = "0.0.0.0";
std::string web_root = "assets/web";
app.add_flag("--web", web_mode, "Run web UI server")->excludes("--task-config")
   ->excludes("--source")->excludes("--target")->excludes("--output")->excludes("--processors");
app.add_option("--web-port", web_port, "Web server port")->check(CLI::Range(1, 65535));
app.add_option("--web-host", web_host, "Web server bind host");
app.add_option("--web-root", web_root, "Frontend static assets root");
```
分发（在现有 if/else 链最前）：
```cpp
} else if (web_mode) {
    exit_code = run_web_mode(web_host, web_port, web_root);
}
```
实现：
```cpp
int App::run_web_mode(const std::string& host, uint16_t port, const std::string& web_root) {
    Logger::get_instance()->info(std::format("Web UI: http://{}:{}/ (root: {})", host, port, web_root));
    app::web::run_server({.host = host, .port = port, .web_root = web_root});
    return 0;
}
```

- [ ] **Step 3: 编译 + 单测回归**

Run: `python build.py --action test --test-label unit`
Expected: 全部通过（无回归）

- [ ] **Step 4: Commit**

```bash
git add src/app/cli/app_cli.cpp src/app/cli/app_cli.ixx src/app/cli/CMakeLists.txt
git commit -m "feat(cli): 新增 --web/--web-port/--web-host/--web-root 参数"
```

---

### Task 4: 前端空壳工程 `web/`

**Files:**
- Create: `web/package.json`、`web/vite.config.ts`、`web/tsconfig.json`、`web/index.html`、`web/src/main.tsx`、`web/src/App.tsx`、`web/src/index.css`
- Create: `web/.gitignore`（node_modules/、dist/）

**Interfaces:**
- Produces: `npm run build` → `web/dist/`（纯静态 SPA）；dev 模式代理 `/api`、`/ws` → `http://127.0.0.1:8000`

- [ ] **Step 1: package.json**

```json
{
  "name": "ffc-web",
  "private": true,
  "version": "0.34.1",
  "type": "module",
  "scripts": {
    "dev": "vite",
    "build": "tsc -b && vite build",
    "preview": "vite preview"
  },
  "dependencies": {
    "react": "^19.1.0",
    "react-dom": "^19.1.0"
  },
  "devDependencies": {
    "@types/react": "^19.1.0",
    "@types/react-dom": "^19.1.0",
    "@vitejs/plugin-react": "^4.4.0",
    "typescript": "~5.8.0",
    "vite": "^6.3.0"
  }
}
```

- [ ] **Step 2: vite.config.ts（含 dev 代理）**

```ts
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    port: 5173,
    proxy: {
      '/api': { target: 'http://127.0.0.1:8000', changeOrigin: true },
      '/ws': { target: 'ws://127.0.0.1:8000', ws: true },
    },
  },
  build: { outDir: 'dist', emptyOutDir: true },
});
```

- [ ] **Step 3: 最小页面**

`web/src/App.tsx`：Header（"ffc Web UI"）+ 占位布局 + `fetch('/api/health')` 展示版本号（验证代理链路）。
`web/index.html`、`main.tsx`、`tsconfig.json`、`index.css` 为标准 Vite React 模板最小集。

- [ ] **Step 4: 验证构建**

Run: `cd web && npm install && npm run build`
Expected: `web/dist/index.html` 生成，无 TS 错误

- [ ] **Step 5: Commit**

```bash
git add web/
git commit -m "feat(web): 前端空壳工程（Vite+React+TS，dev 代理）"
```

---

### Task 5: build.py --action web 与产物同步

**Files:**
- Modify: `build.py`（--action choices 加 "web"；新增 run_web_build()；分发分支）
- Modify: `CMakeLists.txt`（install 增加 assets/web——现有 assets 目录整体 install 已覆盖，若不存在则补充）
- Modify: `.gitignore`（追加 `assets/web/`）

**Interfaces:**
- Produces: `python build.py --action web` → 构建前端并同步产物至 `assets/web/`

- [ ] **Step 1: 实现 run_web_build**

```python
def run_web_build(project_root):
    log("\n=== Action: web ===", "info")
    web_dir = project_root / "web"
    if not (web_dir / "package.json").exists():
        log("web/ not found, skipping", "warning")
        return
    subprocess.run(["npm", "ci"], cwd=web_dir, check=True)
    subprocess.run(["npm", "run", "build"], cwd=web_dir, check=True)
    dist_dir = web_dir / "dist"
    target = project_root / "assets" / "web"
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(dist_dir, target)
    log(f"Web assets synced to {target}", "info")
```
（`import shutil` 已存在则复用；choices 与分发处追加 `"web"`）

- [ ] **Step 2: 验证**

Run: `python build.py --action web`
Expected: `assets/web/index.html` 存在；git status 显示 assets/web 被 gitignore

- [ ] **Step 3: Commit**

```bash
git add build.py .gitignore
git commit -m "feat(build): build.py 新增 --action web 前端构建与产物同步"
```

---

### Task 6: e2e 验证与文档

**Files:**
- Modify: `docs/user/zh/cli_reference.md`、`docs/user/en/cli_reference.md`（新增 `--web` 系列参数表）
- Modify: `docs/dev/zh/architecture/web_ui_design.md`（M1 状态标记）

- [ ] **Step 1: e2e 验证**

Run（在 build/bin/linux-x64-debug 下）：
```bash
./ffc --web --web-port 18090 --web-root ../../../assets/web &
sleep 2
curl -s http://127.0.0.1:18090/api/health    # 期望 {"status":"ok","version":"0.34.1"}
curl -s http://127.0.0.1:18090/ | head -5    # 期望 index.html
kill %1
```

- [ ] **Step 2: 文档更新**

cli_reference.md（zh/en）全局选项表追加：
| `--web` | 无 | 启动 Web 界面服务（与快捷模式/任务配置模式互斥） | `false` |
| `--web-port` | `<port>` | Web 服务端口 | `8000` |
| `--web-host` | `<host>` | Web 服务绑定地址 | `0.0.0.0` |
| `--web-root` | `<path>` | 前端静态资源根目录（开发调试可指向 web/dist） | `assets/web` |

- [ ] **Step 3: Commit**

```bash
git add docs/user/zh/cli_reference.md docs/user/en/cli_reference.md docs/dev/zh/architecture/web_ui_design.md
git commit -m "docs: cli_reference 增补 --web 系列参数"
```

---

## M1 验收清单

- [ ] `ffc --web` 启动后 `/api/health` 返回 ok + 版本
- [ ] 静态托管：访问 `/` 返回前端页面
- [ ] `build.py --action web` 一次命令完成前端构建 + 产物同步
- [ ] 单元测试无回归；WebServerTest 集成测试通过
- [ ] 文档（cli_reference zh/en）同步
- [ ] M1 完成后删除本计划文档（或标记完成），进入 M2 计划制定
