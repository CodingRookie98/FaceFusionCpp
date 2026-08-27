# C++ 任务: 配置接线（WebConfig + parser + app_cli）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 阶段四（3.4）
> **状态**: 进行中

## 目标

将 `persist_dir` 与 `max_execution_seconds` 接入配置系统与 CLI 启动链路，使持久化/超时可通过 `config/app.yaml` 配置。

## 背景

- `WebConfig`（`src/app/config/app_config.ixx`）现有字段 host/port/web_root/temp_dir。
- `config_parser.cpp` web 节解析 4 字段。
- `app_cli.cpp:330` `make_shared<TaskManager>(executor)` 接线点。

## 改动

### 1. WebConfig 新增字段

```cpp
struct WebConfig {
    std::string host = "0.0.0.0";
    uint16_t port = 8000;
    std::string web_root = "assets/web";
    std::string temp_dir = "./temp/uploads";
    std::string persist_dir = "./temp/tasks";      ///< 任务快照持久化目录
    int max_execution_seconds = 3600;              ///< 单任务最长执行时间（秒）
};
```

### 2. config_parser.cpp web 节解析

```cpp
config.web.persist_dir = detail::GetString(web_j, "persist_dir", "./temp/tasks");
config.web.max_execution_seconds = detail::GetInt(web_j, "max_execution_seconds", 3600);
```

### 3. app.yaml 追加

```yaml
web:
  ...
  persist_dir: "./temp/tasks"       # 任务快照持久化目录（服务重启自动恢复排队任务）
  max_execution_seconds: 3600       # 单任务最长执行时间（秒），超时标记 failed
```

### 4. app_cli.cpp run_web_mode 接线

```cpp
auto tasks = std::make_shared<app::web::TaskManager>(
    executor,
    app::web::TaskManagerOptions{
        .persist_dir = app_config.web.persist_dir,
        .max_execution_seconds = app_config.web.max_execution_seconds,
    });
```

## TDD 流程

- **TDD 说明**: 纯配置声明 + 接线，无新业务逻辑；config_parser 的字段解析由现有 config_parser_test.cpp 模式覆盖（若项目对 GetString/GetInt 有测试则扩展；无则手动验证 + 编译）。

### 验证计划

1. `config_parser_test` 扩展：解析含 persist_dir/max_execution_seconds 的 YAML → 字段正确；缺省 → 默认值。
2. 编译通过 + 单元测试回归。
3. 手动验证：`ffc --web` 启动日志确认持久化目录生效；提交任务 → 重启 → 任务恢复。

## 验收标准

- [ ] 配置解析测试通过（或手动验证记录）
- [ ] 编译通过（无警告），单元测试无回归
- [ ] 集成测试 `python build.py --action test --test-label integration` 通过

## 提交信息

```bash
feat(config): wire persist_dir and max_execution_seconds into web config
```