# 项目文档输出实施计划 (Documentation Output Plan)

> **标准参考 & 跨文档链接**:
> *   架构设计文档: [应用层架构设计说明书](../../design.md)
> *   实施路线图: [设计路线图](../../design_roadmap.md)
> *   构建文档: [build.md](../../../build.md)
> *   测试框架: [tests/README.md](../../../../tests/README.md)
> *   关联 TODO: [TODO.md](../../TODO.md)
> *   最后更新: 2026-02-11

## 0. 计划前验证

*   [x] 我已阅读 `docs/dev/design.md` — 完整架构规范。
*   [x] 我已阅读 `docs/dev/design_roadmap.md` — 里程碑清单与模块清单。
*   [x] 我已阅读 `docs/build.md` — 现有构建文档。
*   [x] 我已阅读 `tests/README.md` — 现有测试文档。
*   [x] 我已阅读 `README.md` — 现有项目简介与用户文档入口。
*   [x] 我已阅读 `AGENTS.md` — 开发规范与约束。

**已检查的上下文:**
*   `docs/dev/design.md` — 架构设计、配置 Schema、CLI 规格、Pipeline 设计
*   `docs/dev/design_roadmap.md` — M1-M11 模块清单、硬件验收标准、测试配置模板
*   `docs/build.md` — build.py 用法、CMakePresets、代码风格工具
*   `tests/README.md` — 测试分类、基础设施、最佳实践
*   `README.md` — 项目简介、特性列表、环境要求
*   `src/**/*.ixx` — 100+ 模块接口文件 (架构全景)

## 1. 计划概述

### 1.1 目标与范围

*   **核心目标**: 基于现有项目代码和内部文档，输出面向两类读者的结构化文档体系。
*   **交付物**:
    - **用户文档** (`docs/user/`): 6 篇，面向最终用户
    - **开发者文档** (`docs/dev/`): 7 篇，面向贡献者和二次开发者
*   **语言**: 英文 + 中文双语 (英文为第一语言)
*   **数据来源**: 100% 基于现有代码和 `docs/dev/` 内部文档提炼，不凭空编造功能

### 1.2 关键约束

*   [x] 所有文档内容必须与现有代码实现一致，禁止描述未实现的功能 (除明确标注 *Planned* 外)
*   [x] 用户文档不涉及内部实现细节
*   [x] 开发者文档引用内部代码时使用相对路径
*   [x] 英文为第一语言，中文作为对照翻译

### 1.3 双语文档组织方式

```
docs/user/
├── en/                    # English (Primary)
│   ├── getting_started.md
│   ├── user_guide.md
│   └── ...
└── zh/                    # 中文 (Translation)
    ├── getting_started.md
    └── ...

docs/dev/
├── en/                    # English (Primary)
│   ├── architecture.md
│   ├── module_reference.md
│   └── ...
└── zh/                    # 中文 (Translation)
    ├── architecture.md
    └── ...
```

---

## 2. 文档内容规划

### 2.1 用户文档清单 (`docs/user/`)

> **目标读者**: 下载 Release 包使用的最终用户

| # | 文档 | 内容概要 | 信息来源 |
|---|------|----------|----------|
| 2.1.1 | `getting_started.md` | 环境要求 (CUDA ≥12.2, cuDNN ≥9.2, TensorRT ≥10.2, FFmpeg ≥7.0.2), 下载安装, 首次运行示例 | README.md, design.md 3.4, build.md |
| 2.1.2 | `user_guide.md` | 核心功能介绍 (换脸/增强/表情还原/超分), 图片处理, 视频处理, 批量处理, 处理器组合 | design.md 4.1, design_roadmap.md M6 |
| 2.1.3 | `cli_reference.md` | 所有 CLI 参数详解 (-c, -s, -t, -o, --processors, --system-check, --validate, --log-level), 使用示例 | design.md 3.5.3, app_cli.ixx |
| 2.1.4 | `configuration_guide.md` | app_config.yaml 全字段说明, task_config.yaml 全字段说明, 场景化配置示例 (基础换脸/多处理器/低显存/高端) | design.md 3.1 + 3.2, design_roadmap.md 10.5 |
| 2.1.5 | `hardware_guide.md` | GPU 档位推荐配置 (旗舰≥12GB / 主流8-12GB / 入门4-8GB / 低端<4GB), 显存优化策略, 性能期望 | design.md A.3.3, design_roadmap.md 10.4 |
| 2.1.6 | `faq.md` | 常见报错 (E101-E404) 及解决方案, 性能调优技巧, TensorRT 引擎缓存说明 | design.md 5.3.1, C++_troubleshooting.md |

### 2.2 开发者文档清单 (`docs/dev/`)

> **目标读者**: 项目贡献者, 二次开发者

| # | 文档 | 内容概要 | 信息来源 |
|---|------|----------|----------|
| 2.2.1 | `architecture.md` | 5层架构总览 (Foundation→Platform→Domain→Services→Application), 层间依赖规则, 模块依赖 Mermaid 图, 核心设计决策 (PIMPL, 工厂模式, RAII) | design.md 1-2, design_roadmap.md 0.2 |
| 2.2.2 | `module_reference.md` | 每个 .ixx 模块的职责、公开接口、依赖关系速查表 (~100 模块), 按层分组 | src/**/*.ixx 扫描, design_roadmap.md M1-M10 |
| 2.2.3 | `build_guide.md` | 开发环境搭建 (工具链版本), CMakePresets 说明, vcpkg 依赖管理, 平台差异 (Windows MSVC / Linux GCC), build.py 完整参数 | build.md 增强版, CMakePresets.json |
| 2.2.4 | `testing_guide.md` | 测试框架说明 (unit/integration/benchmark/e2e), 编写新测试规范, Fixture 继承关系, Mock 库使用, 测试运行方法 | tests/README.md 增强版, tests/ 目录结构 |
| 2.2.5 | `contributing.md` | 分支策略 (feature/fix 命名), 提交规范 (build.py test 必须通过), Code Review 流程, TDD 要求, 代码风格 (clang-format/clang-tidy) | AGENTS.md, workflow.md, build.md |
| 2.2.6 | `pipeline_internals.md` | Pipeline 执行流程详解, Processor vs Adapter 职责分离, 队列流控 (Backpressure), Sequential vs Batch 模式, 优雅停机序列 | design.md 4.1-4.2 + 5.6-5.7, pipeline_adapters.ixx |
| 2.2.7 | `ai_inference.md` | InferenceSession ONNX Runtime 封装, TensorRT EP 配置, SessionPool LRU/TTL 机制, ModelRepository 模型管理, 引擎缓存路径策略 | design.md 3.1 inference, design_roadmap.md M3, inference_session.ixx |

---

## 3. 实施路线图

### 3.1 阶段一: 用户文档 — 核心三篇 (P1-Critical)
**目标**: 用户拿到 Release 包后能立即上手使用

*   [x] **任务 1.1**: `getting_started.md` (EN + ZH)
    *   内容: 环境要求表格, 下载安装步骤, 首次运行 (`-s source.jpg -t target.jpg -o ./output/`), 验证输出
*   [x] **任务 1.2**: `cli_reference.md` (EN + ZH)
    *   内容: 完整参数表, 每个参数的使用示例, 参数组合规则 (快捷模式 vs --config 互斥)
*   [x] **任务 1.3**: `configuration_guide.md` (EN + ZH)
    *   内容: app_config.yaml 逐字段说明, task_config.yaml 逐字段说明, 4 个场景化完整示例
*   [ ] **验收标准**:
    *   新用户仅凭这 3 篇文档 + Release 包能完成基础换脸操作
    *   所有配置示例经过实际运行验证

### 3.2 阶段二: 用户文档 — 补充三篇 (P1-Standard)
**目标**: 覆盖进阶使用、硬件适配和问题排查

*   [x] **任务 2.1**: `user_guide.md` (EN + ZH)
    *   内容: 4 大处理器功能介绍 (换脸/增强/表情还原/超分), 图片/视频/批量处理工作流, 处理器组合推荐
*   [x] **任务 2.2**: `hardware_guide.md` (EN + ZH)
    *   内容: GPU 4 档分级配置, 显存优化策略, 性能期望表, 低显存适配建议
*   [x] **任务 2.3**: `faq.md` (EN + ZH)
    *   内容: 按错误码分类 (E1xx 系统/E2xx 配置/E3xx 模型/E4xx 运行时), 性能调优 checklist
*   [ ] **验收标准**:
    *   所有 6 篇用户文档内容完整, 交叉引用正确
    *   `docs/user/en/` 和 `docs/user/zh/` 目录各 6 个文件

### 3.3 阶段三: 开发者文档 — 架构与构建 (P2-High)
**目标**: 新贡献者能理解项目结构并成功构建

*   [x] **任务 3.1**: `architecture.md` (EN + ZH)
    *   内容: 5层架构图 (Mermaid), 层间依赖规则, 设计模式总结, 关键设计决策记录
*   [x] **任务 3.2**: `build_guide.md` (EN + ZH)
    *   内容: 工具链版本要求, 首次构建 step-by-step, CMakePresets 解读, 平台差异表
*   [x] **任务 3.3**: `contributing.md` (EN + ZH)
    *   内容: 分支命名规范, commit 规范, PR 流程, TDD 强制要求, clang-format/tidy 使用
*   [ ] **验收标准**:
    *   新贡献者凭这 3 篇文档能完成: 理解架构 → 搭建环境 → 提交首个 PR

### 3.4 阶段四: 开发者文档 — 深度参考 (P2-Standard)
**目标**: 为二次开发和维护提供深度参考资料

*   [ ] **任务 4.1**: `module_reference.md` (EN + ZH)
    *   内容: ~100 个 .ixx 模块速查表 (模块名, 职责, 公开接口, 依赖), 按 5 层分组
*   [ ] **任务 4.2**: `testing_guide.md` (EN + ZH)
    *   内容: 4 类测试说明 (unit/integration/benchmark/e2e), Fixture 继承图, Mock 使用示例, 新增测试 checklist
*   [ ] **任务 4.3**: `pipeline_internals.md` (EN + ZH)
    *   内容: Pipeline 执行序列图 (Mermaid), Adapter 模式详解, 队列背压机制, Sequential vs Batch 对比
*   [ ] **任务 4.4**: `ai_inference.md` (EN + ZH)
    *   内容: InferenceSession 生命周期, TensorRT EP 配置参数, SessionPool LRU/TTL 图解, 引擎缓存策略
*   [ ] **验收标准**:
    *   所有 7 篇开发者文档完整, 代码引用路径正确
    *   `docs/dev/en/` 和 `docs/dev/zh/` 目录各 7 个文件

---

## 4. 风险管理

| 风险点 | 可能性 | 影响 | 缓解措施 |
|--------|--------|------|----------|
| 代码与文档不一致 | 高 | 误导读者 | 所有示例经过实际运行验证; 文档中标注对应源文件路径 |
| 中文翻译偏差 | 中 | 术语不统一 | 建立术语对照表 (Pipeline=流水线, Adapter=适配器 等); 英文版为权威源 |
| 文档维护成本 | 中 | 文档过时 | 遵循 DRY 原则, 配置 Schema 引用 design.md 而非复制; CONTRIBUTING 中要求代码变更同步更新文档 |
| 模块参考文档工作量大 | 高 | 延期交付 | 脚本自动扫描 .ixx 文件生成初始表格, 人工补充描述 |

---

## 5. 资源与依赖

*   **前置任务**: 无 (文档可独立于代码开发)
*   **关键信息源**:
    - `docs/dev/design.md` — 架构设计权威源
    - `docs/dev/design_roadmap.md` — 模块清单与验收标准
    - `docs/build.md` — 构建系统参考
    - `tests/README.md` — 测试框架参考
    - `AGENTS.md` — 开发规范参考
    - `src/**/*.ixx` — 模块接口定义 (~100 文件)
*   **工具**:
    - Markdown 编辑器
    - Mermaid 图表渲染 (架构图, 序列图)
    - 可选: 脚本扫描 .ixx 文件自动生成模块清单初稿
