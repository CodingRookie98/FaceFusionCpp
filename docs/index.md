# FaceFusionCpp 文档索引 (Documentation Index)

本索引是项目文档知识库的全局导航入口，按 **Diátaxis 六象限**组织。任何新增、删除、移动或重命名操作必须同步更新本索引。

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DOC-INDEX-2026
> - **当前版本 (Version)**: V2.1.0
> **状态 (Status)**: 正式 (Official)
> **权威性 (Authority)**: 规范 (Normative)
> **所有者 (Owner)**: 王辉
> **审核人 (Reviewer)**: 王辉
> **最后更新 (Last Updated)**: 2026-11-20

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V2.1.0** | 2026-11-20 | AI Agent | 王辉 | 登记新增计划 [provider-wiring](./project/plans/provider-wiring/IMPLEMENTATION_PLAN.md)（BL-2/BL-3 闭环），工程文档增至 26 篇。 |
| **V2.0.0** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P2 完成：索引按 Diátaxis 六象限重构，全量登记 73 篇文档；原「用户文档/开发文档」双语分区废止，迁移映射见 [docs-governance 计划](./project/plans/docs-governance/IMPLEMENTATION_PLAN.md)。 |
| **V1.2.8** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P1 立骨架：登记 GOVERNANCE.md、llms.txt、project 与 proposals。 |
| **V1.2.7** | 2026-09-28 | AI Agent | 王辉 | 文档治理 P0 自愈：断链清零、控制头全量合规、孤儿文档收录。 |
| **V1.2.6** | 2026-09-25 | AI Agent | 王辉 | 新增 Vulkan 后端可行性评估报告并登记至评估索引。 |

> 仅保留最近 5 条记录，更早的历史可通过 `git log --oneline docs/index.md` 查阅。

---

## 📁 文档目录拓扑

```text
docs/
├── index.md                 # 本索引（人类总入口）
├── llms.txt                 # AI 智能体机器地图（自动生成）
├── GOVERNANCE.md            # 知识库治理规程
├── proposals/               # 💡 孵化层：RFC 提案
├── tutorials/               # 🎓 教程象限
├── how-to/                  # 🛠️ 操作指南象限
├── reference/               # 📖 技术参考象限
├── explanation/             # 💡 深度剖析象限
├── project/                 # 🚀 工程演进与项目管理
├── templates/               # 文档模板
└── archived/                # 已废止文档
```

## 🧭 知识库治理 (Governance)

| 文档 | 说明 |
| :--- | :--- |
| [GOVERNANCE.md](./GOVERNANCE.md) | 知识库治理规程：拓扑职责、控制头、链接规则、门禁与生命周期 |
| [llms.txt](./llms.txt) | AI 智能体机器地图（由 `generate-llms-txt.py` 自动生成，勿手改） |

## 🎓 教程 (Tutorials)

> 学习导向：新手从零到首次成功

| 文档 | 标题 |
| :--- | :--- |
| [getting_started.md](./tutorials/en/getting_started.md) | Getting Started |
| [getting_started.md](./tutorials/zh/getting_started.md) | 快速开始 (Getting Started) |

## 🛠️ 操作指南 (How-To Guides)

> 任务导向：完成具体任务的步骤与排障手册

| 文档 | 标题 |
| :--- | :--- |
| [faq.md](./how-to/en/faq.md) | Frequently Asked Questions (FAQ) |
| [setup.md](./how-to/en/setup.md) | Setup & Build Guide |
| [README.md](./how-to/en/troubleshooting/README.md) | Troubleshooting Knowledge Base |
| [cpp20_modules.md](./how-to/en/troubleshooting/issues/cpp20_modules.md) | Issue: C++20 Modules Not Supported (Compiler) |
| [trt_myelin_crash.md](./how-to/en/troubleshooting/issues/trt_myelin_crash.md) | Issue: TensorRT Myelin Crash on Exit |
| [video_timeout.md](./how-to/en/troubleshooting/issues/video_timeout.md) | Issue: Video Test Timeout (Strict Memory) |
| [videowriter_dimensions.md](./how-to/en/troubleshooting/issues/videowriter_dimensions.md) | Issue: Invalid Frame Dimensions (VideoWriter) |
| [user_guide.md](./how-to/en/user_guide.md) | User Guide |
| [web_guide.md](./how-to/en/web_guide.md) | Web UI Guide |
| [CI_CD_recommendations.md](./how-to/zh/CI_CD_recommendations.md) | CI/CD 开发与运行建议 |
| [build.md](./how-to/zh/build.md) | 使用 `build.py` 脚本配置和构建 |
| [faq.md](./how-to/zh/faq.md) | 常见问题解答 (FAQ) |
| [setup.md](./how-to/zh/setup.md) | 环境搭建与构建指南 (Setup & Build Guide) |
| [README.md](./how-to/zh/troubleshooting/README.md) | 故障排查知识库 (Troubleshooting Knowledge Base) |
| [cpp20_modules.md](./how-to/zh/troubleshooting/issues/cpp20_modules.md) | Issue: C++20 Modules 不被支持 (Compiler Support) |
| [trt_myelin_crash.md](./how-to/zh/troubleshooting/issues/trt_myelin_crash.md) | Issue: TensorRT Myelin 进程退出崩溃 (Process Exit Crash) |
| [video_timeout.md](./how-to/zh/troubleshooting/issues/video_timeout.md) | Issue: 视频测试超时 (Strict Memory Timeout) |
| [videowriter_dimensions.md](./how-to/zh/troubleshooting/issues/videowriter_dimensions.md) | Issue: VideoWriter 帧尺寸无效 (Invalid Frame Dimensions) |
| [user_guide.md](./how-to/zh/user_guide.md) | 用户指南 (User Guide) |
| [vcpkg_best_practice.md](./how-to/zh/vcpkg_best_practice.md) | vcpkg 最佳实践 (vcpkg Best Practice) |
| [web_guide.md](./how-to/zh/web_guide.md) | Web 界面使用指南 (Web UI Guide) |

## 📖 技术参考 (Reference)

> 信息导向：配置项、CLI、规则等机械事实真相源

| 文档 | 标题 |
| :--- | :--- |
| [cli_reference.md](./reference/en/cli_reference.md) | CLI Reference |
| [configuration_guide.md](./reference/en/configuration_guide.md) | Configuration Guide |
| [hardware_guide.md](./reference/en/hardware_guide.md) | Hardware Guide |
| [cli_reference.md](./reference/zh/cli_reference.md) | 命令行参考 (CLI Reference) |
| [configuration_guide.md](./reference/zh/configuration_guide.md) | 配置指南 (Configuration Guide) |
| [hardware_guide.md](./reference/zh/hardware_guide.md) | 硬件指南 (Hardware Guide) |
| [C++_quality_standard.md](./reference/zh/rules/C++_quality_standard.md) | C++ 代码质量与工程化标准（FaceFusionCpp） |
| [workflow.md](./reference/zh/rules/workflow.md) | 工作流程 (Workflow) |

## 💡 深度剖析 (Explanation)

> 理解导向：架构设计、评估分析与决策记录

| 文档 | 标题 |
| :--- | :--- |
| [design.md](./explanation/en/architecture/design.md) | Application Layer Architecture Design (V2.10) |
| [layers.md](./explanation/en/architecture/layers.md) | Layered Architecture Implementation Details |
| [web_ui_design.md](./explanation/en/architecture/web_ui_design.md) | Web User Interface (Web UI) Design Specification |
| [architecture-performance-review.md](./explanation/zh/analysis/architecture-performance-review.md) | 推理架构与算法性能评估报告 |
| [vulkan-backend-feasibility-review.md](./explanation/zh/analysis/vulkan-backend-feasibility-review.md) | Vulkan 后端支持可行性评估报告 |
| [web-ui-design-completion-review.md](./explanation/zh/analysis/web-ui-design-completion-review.md) | Web UI 设计文档（web_ui_design.md）M1-M4 完成度对抗性复核报告 |
| [design-appendix.md](./explanation/zh/architecture/design-appendix.md) | 设计附录 (Design Appendix) |
| [design-configuration.md](./explanation/zh/architecture/design-configuration.md) | 配置体系设计 (Configuration Design) |
| [design-engineering.md](./explanation/zh/architecture/design-engineering.md) | 工程化约束与最佳实践 (Engineering Constraints) |
| [design-interfaces.md](./explanation/zh/architecture/design-interfaces.md) | 接口与构建规范 (Interfaces & Build Specification) |
| [design-pipeline.md](./explanation/zh/architecture/design-pipeline.md) | 核心业务处理逻辑 (Core Business Logic) |
| [design.md](./explanation/zh/architecture/design.md) | 应用层架构设计说明书 (Application Layer Architecture Design Specification) |
| [layers.md](./explanation/zh/architecture/layers.md) | 分层架构实现细节 (Layered Architecture Implementation) |
| [web_ui_design.md](./explanation/zh/architecture/web_ui_design.md) | Web 界面（Web UI）设计规格 |
| [compiler_selection.md](./explanation/zh/decisions/compiler_selection.md) | 编译器选型技术决策记录 (Compiler Selection ADR) |

## 💡 需求孵化 (Proposals / RFC)

> 尚未定案的提案集中对齐空间

| 文档 | 标题 |
| :--- | :--- |
| [README.md](./proposals/README.md) | 需求与设计孵化层 (Proposals / RFC) |

## 🚀 工程演进 (Project)

> 路线图、待办、变更日志与历史计划归档

| 文档 | 标题 |
| :--- | :--- |
| [backlog.md](./project/backlog.md) | 待办与技术债汇总 (Backlog) |
| [changelog.md](./project/changelog.md) | 版本变更日志索引 (Changelog Index) |
| [IMPLEMENTATION_PLAN.md](./project/plans/docs-governance/IMPLEMENTATION_PLAN.md) | 实施计划：文档治理 Diátaxis 标准化迁移 |
| [IMPLEMENTATION_PLAN.md](./project/plans/perf-p0/IMPLEMENTATION_PLAN.md) | 性能优化 P0 批次 实施计划 |
| [C++_task_domain_service_reuse.md](./project/plans/perf-p0/task/C++_task_domain_service_reuse.md) | C++ 任务: domain 服务跨视频复用（消除每视频 ONNX 重解析） |
| [C++_task_face_store_cache_disable.md](./project/plans/perf-p0/task/C++_task_face_store_cache_disable.md) | C++ 任务: 视频路径禁用 FaceStore 整帧哈希 |
| [C++_task_inswapper_embedding_cache.md](./project/plans/perf-p0/task/C++_task_inswapper_embedding_cache.md) | C++ 任务: InSwapper embedding 变换任务级缓存 |
| [C++_task_session_pool_capacity.md](./project/plans/perf-p0/task/C++_task_session_pool_capacity.md) | C++ 任务: SessionPool 容量修正（max_entries 3→10） |
| [IMPLEMENTATION_PLAN.md](./project/plans/perf-p1a/IMPLEMENTATION_PLAN.md) | 性能优化 P1 批次（第一批）实施计划 |
| [C++_task_gpu_concurrency_gate.md](./project/plans/perf-p1a/task/C++_task_gpu_concurrency_gate.md) | C++ 任务: GPU 并发信号量闸门（max_concurrent_gpu_tasks） |
| [C++_task_session_key_fingerprint.md](./project/plans/perf-p1a/task/C++_task_session_key_fingerprint.md) | C++ 任务: session key 加入模型文件指纹 |
| [C++_task_session_ttl_lazy_cleanup.md](./project/plans/perf-p1a/task/C++_task_session_ttl_lazy_cleanup.md) | C++ 任务: SessionPool TTL 惰性清理 |
| [C++_task_strict_queue_capacity.md](./project/plans/perf-p1a/task/C++_task_strict_queue_capacity.md) | C++ 任务: Strict 模式 queue 上限放宽 |
| [IMPLEMENTATION_PLAN.md](./project/plans/perf-p1b/IMPLEMENTATION_PLAN.md) | 性能优化 P1 批次（第二批）实施计划 |
| [C++_task_mask_chain_wiring.md](./project/plans/perf-p1b/task/C++_task_mask_chain_wiring.md) | C++ 任务: mask 链路接线 + 共享（P1-2 方案 B） |
| [C++_task_session_pool_lock.md](./project/plans/perf-p1b/task/C++_task_session_pool_lock.md) | C++ 任务: SessionPool 池锁重构（factory 移出锁） |
| [C++_task_strict_checkpoint_throttle.md](./project/plans/perf-p1b/task/C++_task_strict_checkpoint_throttle.md) | C++ 任务: Strict 模式 checkpoint 周期保存 |
| [C++_task_web_ws_flaky.md](./project/plans/perf-p1b/task/C++_task_web_ws_flaky.md) | C++ 任务: Web 集成 flaky 修复（WS 超时放宽） |
| [IMPLEMENTATION_PLAN.md](./project/plans/perf-p2minor/IMPLEMENTATION_PLAN.md) | 性能优化 P2 小项 实施计划 |
| [IMPLEMENTATION_PLAN.md](./project/plans/provider-wiring/IMPLEMENTATION_PLAN.md) | 推理 Provider 配置接线（BL-2 + BL-3）实施计划 |
| [IMPLEMENTATION_PLAN.md](./project/plans/web-robustness/IMPLEMENTATION_PLAN.md) | Web 任务持久化与超时机制 实施计划 |
| [C++_task_config_serialize.md](./project/plans/web-robustness/task/C++_task_config_serialize.md) | C++ 任务: TaskConfig/TaskEntry JSON 序列化 |
| [C++_task_config_wiring.md](./project/plans/web-robustness/task/C++_task_config_wiring.md) | C++ 任务: 配置接线（WebConfig + parser + app_cli） |
| [C++_task_persistence.md](./project/plans/web-robustness/task/C++_task_persistence.md) | C++ 任务: TaskManager 任务持久化集成 |
| [C++_task_timeout.md](./project/plans/web-robustness/task/C++_task_timeout.md) | C++ 任务: TaskManager 任务超时机制 |
| [roadmap.md](./project/roadmap.md) | 产品规划路线图 (Roadmap) |

## 📄 文档模板 (Templates)

> 按规范创建新文档的脚手架

| 文档 | 标题 |
| :--- | :--- |
| [C++_plan_template.md](./templates/C++_plan_template.md) | {计划名称} 实施计划 |
| [C++_task_template.md](./templates/C++_task_template.md) | {任务名称} 实施任务单 |

## 📦 归档 (Archived)

> 已废止文档（保留可追溯性）

| 文档 | 标题 |
| :--- | :--- |
| [C++_evaluation_template.md](./archived/C++_evaluation_template.md) | C++ 代码质量评估报告模板（已废止归档） |
| [quality-scoring-rubric.md](./archived/quality-scoring-rubric.md) | 代码质量评分标准（已废止归档） |
| [quality.md](./archived/quality.md) | C++ Quality Standards & Development Checklist |

## 📐 治理规范

- 治理规则与门禁详见 [GOVERNANCE.md](./GOVERNANCE.md)；
- 所有文档必须包含「文档控制信息」与「修订历史记录」头部模块（修订历史最多保留最近 5 条）；
- 内部交叉引用必须采用显式物理文件链接，显示文本保留 `.md` 后缀；
- 新增/移动/删除文档后必须同步更新本索引与 [llms.txt](./llms.txt)。
