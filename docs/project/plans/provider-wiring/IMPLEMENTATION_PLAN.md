# 推理 Provider 配置接线（BL-2 + BL-3）实施计划

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PLAN-PROVIDER-WIRING-2026
> - **当前版本 (Version)**: V1.1.0
> - **状态 (Status)**: 已完成 (Completed)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-11-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.0** | 2026-11-20 | AI Agent | 王辉 | 验收完成：新增用例 10/10 绿，全量单元 329/329 绿；`pipeline_runner` 与 `app_cli` 两处写死调用已消除；`device_id` 与 `engine_cache.enable/path` 因与 `Options` 1:1 一并接线（`max_entries`/`idle_timeout_seconds` 另立 BL-9）。 |
| **V1.0.0** | 2026-11-20 | AI Agent | 王辉 | 立项：闭合 BL-2（`inference.default_providers` 解析后从未映射为 `ExecutionProvider`）与 BL-3（用户 FAQ 与实现不一致）；登记语义裁决 D1–D4、TDD 用例清单与验收口径。用户已选定方案 B 并授权开工。 |

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../../AGENTS.md)
> *   工作流程: [workflow.md](../../../reference/zh/rules/workflow.md)
> *   缺陷来源: [vulkan-backend-feasibility-review.md](../../../explanation/zh/analysis/vulkan-backend-feasibility-review.md) §3.2
> *   待办台账: [backlog.md](../../backlog.md) BL-2 / BL-3

## 0. 计划前验证 (AI Agent 自检)

*   [x] 已阅读 [AGENTS.md](../../../../AGENTS.md) 的 C++20 开发规范与测试分层策略。
*   [x] 已阅读相关现有模块接口与调用点（见下"已检查的上下文"）。
*   [x] 已确认新增接口不与现有符号冲突（`config` 命名空间下无同名函数）。
*   [x] **TDD 承诺**: 先写失败测试（Red），再最小实现（Green），最后在测试保护下重构。

**已检查的上下文:**

*   `src/app/config/app_config.ixx:39-40` —— `InferenceConfig::default_providers` 默认 `{"tensorrt","cuda","cpu"}`。
*   `src/app/config/parser/config_parser.cpp:330-333` —— YAML 解析入结构体，**此后无任何消费点**。
*   `src/services/pipeline/pipeline_runner.cpp:88` —— `m_inference_options = Options::with_best_providers();`（忽略配置）。
*   `src/app/cli/app_cli.cpp:349-350` —— Web 人脸检测路径同样写死 `with_best_providers()`。
*   `src/foundation/ai/inference_session.cpp:37-66` —— `get_best_available_providers()` 返回**闭包**：TRT ⇒ {TRT, CUDA, CPU}，CUDA ⇒ {CUDA, CPU}，否则 {CPU}。
*   `src/foundation/ai/inference_session.cpp:305-318` —— `Options::execution_providers` 为空时自动探测；非空时按集合 append（TRT/CUDA 各自 try-catch）。

## 1. 目标与范围

### 1.1 目标

1. **BL-2**: 让 `inference.default_providers` 真正决定会话的 `ExecutionProvider` 集合，两条生产路径（PipelineRunner、Web 人脸检测）统一走同一映射。
2. **BL-3**: 用户文档（FAQ zh/en、configuration_guide zh/en）描述与实现对齐。

### 1.2 范围内

*   新增 `config.inference_options` 模块：`parse_execution_provider` / `resolve_execution_providers` / `make_inference_options`。
*   接线 `PipelineRunner::Impl` 与 `app_cli` 的 detect_faces 路径。
*   顺带接线 `inference.device_id` 与 `inference.engine_cache.enable/path`（与 `Options` 字段 1:1，属同一"解析未消费"缺陷）。
*   单元测试（新测试文件 + 注册）。

### 1.3 范围外（不做）

*   `engine_cache.max_entries` / `idle_timeout_seconds`（`Options` 无对应字段，需 `SessionPool` 侧接收；已登记 BL-9）。
*   `face_masker_factory.ixx` / `face_test_support.ixx` 中的 `with_best_providers()` 调用（Domain 层默认值，非 App 配置路径）。
*   `FACEFUSION_ENABLE_GPU` 空转开关（BL-5，独立事项）。

## 2. 语义裁决（本项目权威口径）

| ID | 裁决 | 理由 |
| :--- | :--- | :--- |
| **D1** | 配置是**优先级序列**，取"第一个既被配置又可用的 provider"，再展开为闭包（TRT ⇒ {TRT,CUDA,CPU}；CUDA ⇒ {CUDA,CPU}；CPU ⇒ {CPU}） | 与 `get_best_available_providers()` 的既有闭包语义一致；避免配置里写 TRT 但在无 TRT 环境抛错降级为 CPU-only |
| **D2** | 配置项大小写不敏感，仅接受 `tensorrt` / `cuda` / `cpu` | 仅支持 ORT 实际可用的三者；不引入别名（避免文档与实现口径二义） |
| **D3** | 未知名称**忽略**；若全部未知或列表为空 → 回退自动探测（`get_best_available_providers()`） | 保持向后兼容：旧/错配置不会让程序起不来，且与 `Options` 的"空 = 自动"既有契约一致 |
| **D4** | 可用性判定复用 `get_best_available_providers()` 的结果集，通过参数注入到纯函数中以获得可测试性 | 可在无 GPU 的 CI/单测环境验证全部优先级分支 |

## 3. TDD 用例清单（先红后绿）

测试文件：`tests/unit/app/config/inference_options_test.cpp`（新）；链接 `config_core` + `GTest::gmock`。

| # | 用例 | 断言 |
| :--- | :--- | :--- |
| T1 | `parse_execution_provider` 识别大小写变体 | `"tensorrt"/"TensorRT"/"TRT"` → TensorRT；`"cuda"` → CUDA；`"cpu"` → CPU |
| T2 | 未知名称 | `"rocm"` → `std::nullopt` |
| T3 | 显式 CPU 优先 | `resolve({"cpu"}, {TRT,CUDA,CPU})` → `{CPU}` |
| T4 | 跳过不可用的高优先级项 | `resolve({"tensorrt","cuda","cpu"}, {CUDA,CPU})` → `{CUDA,CPU}` |
| T5 | TRT 命中时展开闭包 | `resolve({"tensorrt"}, {TRT,CUDA,CPU})` → `{TRT,CUDA,CPU}` |
| T6 | 空配置回退自动 | `resolve({}, {CUDA,CPU})` → `{CUDA,CPU}` |
| T7 | 全部未知回退自动 | `resolve({"rocm","directml"}, {CPU})` → `{CPU}` |
| T8 | 配置→Options 端到端 | `make_inference_options(InferenceConfig{.default_providers={"cpu"}})` → `options.execution_providers == {CPU}` 且 `execution_device_id` 透传 |

## 4. 任务分解

| 任务 | 名称 | 说明 | 依赖 |
| :--- | :--- | :--- | :--- |
| T-A | 测试先行 | 新建 `inference_options_test.cpp` + 注册到 `tests/unit/app/CMakeLists.txt`；确认编译失败/断言失败（Red） | - |
| T-B | 映射模块 | 新建 `src/app/config/inference_options.ixx`（`config.inference_options`），加入 `config_core` 模块集合并链接 `foundation_ai` | T-A |
| T-C | 生产接线 | `pipeline_runner.cpp` 与 `app_cli.cpp` 改用 `config::make_inference_options(...)` | T-B |
| T-D | 文档对齐 | FAQ（zh/en）、configuration_guide（zh/en）、backlog（BL-2/BL-3 闭环 + 新增 BL-9）、index 登记 | T-C |

## 5. 验收标准

1. 新增用例 T1–T8 全绿；全量单元测试零失败（基线 319，新增 10 条 → 实测 329）。
2. 构建零警告新增（`-Wall -Wextra -Wpedantic`）。
3. `grep` 复核：`with_best_providers()` 不再出现在 `src/services/pipeline/pipeline_runner.cpp` 与 `src/app/cli/app_cli.cpp`。
4. 文档门禁：`check-doc-links.py` / `audit-doc-health.py` / `trim-revision.py` 全 PASS。
5. BL-2、BL-3 在 [backlog.md](../../backlog.md) 移入"已闭环"并附证据。

### 5.1 验收记录（2026-11-20）

| 检查项 | 命令 / 位置 | 结果 |
| :--- | :--- | :--- |
| 新用例（Red→Green） | `./build/bin/linux-x64-debug/inference_options_test` | 10 / 10 PASSED（先验红：`module 'config.inference_options' not found`） |
| 全量单元 | `ctest --test-dir build/linux-x64-debug -L unit` | 329 passed / 0 failed（1 Disabled；基线 319 + 新增 10） |
| 全量构建 | `python3 build.py --action build --no-web` | 成功，含 `ffc` 与全部测试二进制；无新增警告 |
| 写死调用消除 | `grep -n with_best_providers src/services/pipeline/pipeline_runner.cpp src/app/cli/app_cli.cpp` | 0 匹配 |
| 集成（快速子集） | `component_integration_tests`（FaceHelper/FaceSelector/Face）+ `web_api/web_ws/web_server` | 18 + 19 + 2 + 1 全绿，0 失败 |
| 集成（模型推理）/ E2E | `ctest -L integration -E app_pipeline_tests` / `run_e2e.py` | **未执行**：单用例真实推理 1–3 分钟；`assets/models` 缺 `inswapper_128_fp16`、`lenna.bmp` 等素材 |
| 文档门禁 | `scripts/doc/check-doc-links.py` / `audit-doc-health.py` / `trim-revision.py` | 全 PASS：80 篇扫描、0 断链、修订窗口合规、健康度"未检测到缺陷" |
| 代码格式 | `clang-format --dry-run --Werror`（新增/改动文件） | 无违规 |

> 备注：首轮全量单元曾出现 `web_task_manager_test.TaskManagerTest.FailedTaskRecordsError` 偶发失败，单跑与整轮复跑均通过，属既存 flaky（与本变更无关，未在本次处理）。

## 6. 风险与回退

| 风险 | 影响 | 处置 |
| :--- | :--- | :--- |
| `config_core` 新增对 `foundation_ai` 的 PUBLIC 链接导致编译面扩大 | 构建时间略增 | 属 App → Foundation 合法直连（见 [layers.md](../../../explanation/zh/architecture/layers.md) §依赖规则 2） |
| 用户配置了本机不可用的 TRT，旧行为是尝试后降级 | 行为变化（新行为直接跳过） | D1 裁决：跳过更可预期；日志打印实际生效 provider |
| 显式配置 `cpu` 导致 GPU 环境性能骤降 | 用户误配 | FAQ 明示"删掉该键即自动选择最优" |

回退路径：单 commit 回退到 `fix/task-inference-provider-wiring` 分支起点（`dev` @ `84492a2`），无数据迁移、无契约破坏。
