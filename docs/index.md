# FaceFusionCpp 文档索引 (Documentation Index)

本索引是项目文档知识库的全局导航入口。所有存放在 `docs/` 目录下的文档（含子目录）均在此登记，任何新增、删除、移动或重命名操作必须同步更新本索引。

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DOC-INDEX-2026
> - **当前版本 (Version)**: V1.1.8
> **状态 (Status)**: 正式 (Official)
> **权威性 (Authority)**: 规范 (Normative)
> **所有者 (Owner)**: 王辉
> **审核人 (Reviewer)**: 王辉
> **最后更新 (Last Updated)**: 2026-08-18

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.1.8** | 2026-08-18 | AI Agent | 王辉 | 登记新增英文设计文档 web_ui_design.md（Web UI Design Specification）。 |
| **V1.1.7** | 2026-08-17 | AI Agent | 王辉 | 登记新增评估报告 web-ui-design-completion-review.md（Web UI M1-M4 完成度对抗性复核，待审批）。 |
| **V1.1.6** | 2026-08-17 | AI Agent | 王辉 | 同步 M4 完成状态：更新 web_ui_design.md 与 web_guide.md（zh/en）。 |
| **V1.1.5** | 2026-08-17 | AI Agent | 王辉 | 登记 Web 界面使用指南（zh/en）与设计规格状态更新。 |
| **V1.1.4** | 2026-08-14 | AI Agent | 王辉 | 登记新增架构设计文档 web_ui_design.md（Web 界面设计规格）。 |
| **V1.1.2** | 2026-08-14 | AI Agent | 王辉 | 登记新增评估文档 layers_doc_assessment.md（分层架构文档一致性评估报告）。 |
| **V1.1.1** | 2026-08-13 | AI Agent | 王辉 | 登记新增评估文档 design_doc_assessment.md（功能设计文档完整性评估报告）。 |
| **V1.1.0** | 2026-08-13 | AI Agent | 王辉 | 合并 dev 分支后更新索引：删除 quality.md（并入 C++_quality_standard.md），新增 evaluation/plan/templates/superpowers/compiler_selection 等 18 篇文档。 |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范创建全局索引，收录全部 35 篇文档；修复全仓断链并统一文档目录结构。 |

> 仅保留最近 5 条记录，更早的历史可通过 `git log --oneline docs/index.md` 查阅。

---

## 📁 文档目录拓扑

```
docs/
├── index.md                    # 本索引
├── user/                       # 用户文档（面向终端用户）
│   ├── zh/                     #   中文
│   └── en/                     #   英文
└── dev/                        # 开发文档（面向开发者与 AI Agent）
    ├── zh/                     #   中文
    │   ├── architecture/       #     架构核心
    │   ├── process/            #     流程军规
    │   ├── guides/             #     构建与环境指南
    │   └── troubleshooting/    #     疑难杂症
    │       └── issues/         #       具体问题记录
    └── en/                     #   英文（结构与 zh 对齐，部分文档仅中文）
```

## 📖 用户文档 (User Documentation)

### 中文 (Chinese)

| 文档 | 说明 |
| :--- | :--- |
| [getting_started.md](./user/zh/getting_started.md) | 快速上手：环境初探与首次运行 |
| [user_guide.md](./user/zh/user_guide.md) | 功能介绍与操作指南 |
| [web_guide.md](./user/zh/web_guide.md) | Web 界面使用指南 |
| [configuration_guide.md](./user/zh/configuration_guide.md) | 核心配置参数说明 (`app_config.yaml`, `task_config.yaml`) |
| [cli_reference.md](./user/zh/cli_reference.md) | 命令行参数详解 |
| [hardware_guide.md](./user/zh/hardware_guide.md) | 硬件性能优化建议 |
| [faq.md](./user/zh/faq.md) | 常见问题解答 |

### English

| 文档 | 说明 |
| :--- | :--- |
| [getting_started.md](./user/en/getting_started.md) | Quick start: setup and first run |
| [user_guide.md](./user/en/user_guide.md) | Feature introduction and operation guide |
| [web_guide.md](./user/en/web_guide.md) | Web UI Guide |
| [configuration_guide.md](./user/en/configuration_guide.md) | Core configuration parameters (`app_config.yaml`, `task_config.yaml`) |
| [cli_reference.md](./user/en/cli_reference.md) | Command-line arguments reference |
| [hardware_guide.md](./user/en/hardware_guide.md) | Hardware performance optimization advice |
| [faq.md](./user/en/faq.md) | Frequently Asked Questions |

## 🛠️ 开发文档 (Developer Documentation)

### 中文 (Chinese)

**架构 (Architecture)**

| 文档 | 说明 |
| :--- | :--- |
| [design.md](./dev/zh/architecture/design.md) | 系统设计与原则（总纲） |
| [layers.md](./dev/zh/architecture/layers.md) | 分层结构实现细节 |
| [web_ui_design.md](./dev/zh/architecture/web_ui_design.md) | Web 界面设计规格（M1-M4 已完成） |

**流程 (Process)**

| 文档 | 说明 |
| :--- | :--- |
| [workflow.md](./dev/zh/process/workflow.md) | 开发流水线 Checklist（必读） |
| [C++_quality_standard.md](./dev/zh/process/C++_quality_standard.md) | C++ 代码规范与工程化标准（含原 quality.md 内容） |
| [vcpkg_best_practice.md](./dev/zh/process/vcpkg_best_practice.md) | vcpkg 最佳实践 |
| [CI_CD_recommendations.md](./dev/zh/process/CI_CD_recommendations.md) | CI/CD 开发与运行建议 |

**指南 (Guides)**

| 文档 | 说明 |
| :--- | :--- |
| [setup.md](./dev/zh/guides/setup.md) | 技术构建环境搭建指南 |
| [build.md](./dev/zh/guides/build.md) | 使用 `build.py` 脚本配置和构建 |
| [compiler_selection.md](./dev/zh/guides/compiler_selection.md) | 编译器选型决策记录 (ADR) |

**评估 (Evaluation)**

| 文档 | 说明 |
| :--- | :--- |
| [web-ui-design-completion-review.md](./dev/zh/evaluation/web-ui-design-completion-review.md) | Web UI M1-M4 完成度对抗性复核报告（待审批） |

**模板 (Templates)**

| 文档 | 说明 |
| :--- | :--- |
| [C++_evaluation_template.md](./dev/templates/C++_evaluation_template.md) | 评估文档模板 |
| [C++_plan_template.md](./dev/templates/C++_plan_template.md) | 计划文档模板 |
| [C++_task_template.md](./dev/templates/C++_task_template.md) | 任务文档模板 |

**故障排查 (Troubleshooting)**

| 文档 | 说明 |
| :--- | :--- |
| [README.md](./dev/zh/troubleshooting/README.md) | 疑难杂症分级检索索引 |
| [issues/cpp20_modules.md](./dev/zh/troubleshooting/issues/cpp20_modules.md) | C++20 Modules 编译支持问题 |
| [issues/trt_myelin_crash.md](./dev/zh/troubleshooting/issues/trt_myelin_crash.md) | TensorRT Myelin 进程退出崩溃 |
| [issues/video_timeout.md](./dev/zh/troubleshooting/issues/video_timeout.md) | 视频测试超时（严格内存模式） |
| [issues/videowriter_dimensions.md](./dev/zh/troubleshooting/issues/videowriter_dimensions.md) | VideoWriter 帧尺寸无效 |

### English

**Architecture**

| 文档 | 说明 |
| :--- | :--- |
| [design.md](./dev/en/architecture/design.md) | System design and principles (overview) |
| [layers.md](./dev/en/architecture/layers.md) | Layered architecture implementation details |
| [web_ui_design.md](./dev/en/architecture/web_ui_design.md) | Web UI design specification |

**Process**

| 文档 | 说明 |
| :--- | :--- |
| [quality.md](./dev/en/process/quality.md) | Quality standards and development checklist |

> ℹ️ `workflow.md`、`C++_quality_standard.md`、`vcpkg_best_practice.md`、`CI_CD_recommendations.md` 目前仅提供中文版，参见 [中文索引](#中文-chinese)。

**Guides**

| 文档 | 说明 |
| :--- | :--- |
| [setup.md](./dev/en/guides/setup.md) | Environment setup and build guide |

**Troubleshooting**

| 文档 | 说明 |
| :--- | :--- |
| [README.md](./dev/en/troubleshooting/README.md) | Troubleshooting knowledge base index |
| [issues/cpp20_modules.md](./dev/en/troubleshooting/issues/cpp20_modules.md) | C++20 Modules compiler support issue |
| [issues/trt_myelin_crash.md](./dev/en/troubleshooting/issues/trt_myelin_crash.md) | TensorRT Myelin crash on exit |
| [issues/video_timeout.md](./dev/en/troubleshooting/issues/video_timeout.md) | Video test timeout (strict memory) |
| [issues/videowriter_dimensions.md](./dev/en/troubleshooting/issues/videowriter_dimensions.md) | Invalid frame dimensions (VideoWriter) |

---

## 📐 治理规范

- 文档治理规则详见根目录 [AGENTS.md](../AGENTS.md) 的「📝 文档维护规范 (Documentation Standards)」章节。
- 所有文档必须包含「文档控制信息」与「修订历史记录」头部模块（修订历史最多保留最近 5 条）。
- 内部交叉引用必须采用显式物理文件链接，显示文本保留 `.md` 后缀。
