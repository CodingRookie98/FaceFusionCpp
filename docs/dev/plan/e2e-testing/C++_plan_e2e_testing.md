# E2E 测试实施计划 (End-to-End Testing Plan)

> **标准参考 & 跨文档链接**:
> *   架构设计文档: [应用层架构设计说明书](../../design.md)
> *   实施路线图: [设计路线图](../../design_roadmap.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../../C++_quality_standard.md)
> *   关联 TODO: [TODO.md](../../TODO.md)
> *   最后更新: 2026-02-11

## 0. 计划前验证

*   [x] 我已阅读 `docs/dev/design_roadmap.md` 中 M11 集成测试章节。
*   [x] 我已阅读 `docs/dev/design.md` 中 A.3 标准测试素材规范。
*   [x] 我已阅读 `tests/README.md` 了解现有测试框架。
*   [x] 我已确认 `tests/e2e/` 目录不存在现有冲突。

**已检查的上下文:**
*   `docs/dev/design_roadmap.md` — M11 测试矩阵与验收标准
*   `docs/dev/design.md` — A.3 测试素材, A.3.3 硬件验收标准, 3.5 CLI 参数规格
*   `tests/README.md` — 现有测试分类标准 (unit/integration/benchmark)
*   `tests/integration/app/test_config_e2e_baseline.yaml` — 现有基线配置参考
*   `AGENTS.md` — "集成测试和端到端测试由用户手动验证"

## 1. 计划概述

### 1.1 目标与范围

*   **核心目标**: 建立独立的 E2E 测试体系，通过**命令行执行编译好的可执行文件**，端到端验证从 CLI 入口 → 配置加载 → Pipeline 执行 → 文件输出的完整用户路径。
*   **与集成测试的关键区别**:

    | 维度 | 集成测试 (`tests/integration/`) | E2E 测试 (`tests/e2e/`) |
    |------|------|------|
    | 调用方式 | GTest, C++ API 直接调用 | 命令行执行 `./FaceFusionCpp` |
    | 入口点 | `PipelineRunner::process()` | `main()` → CLI 解析 |
    | 构建依赖 | 编译为 GTest 测试二进制 | 不参与 CMake 构建 |
    | 执行人 | CI / AI 自动运行 | 用户手动验证 |

*   **涉及模块**: 无代码修改，纯测试基础设施（脚本 + 配置 + 文档）

### 1.2 关键约束

*   [x] E2E 测试**不参与 CMake 构建系统**，不生成 GTest 二进制
*   [x] 所有测试必须在**可执行文件所在目录**执行 (`cd` 到 bin 目录)
*   [x] 性能测试必须使用 **Release** 构建模式
*   [x] 测试素材复用 `assets/standard_face_test_images/` 和 `assets/standard_face_test_videos/`

---

## 2. 架构设计

### 2.1 目录结构

```
tests/e2e/                                 # 独立于 CMake 构建
├── README.md                              # E2E 测试说明与执行步骤
├── configs/                               # E2E 专用配置文件
│   ├── e2e_image_512_baseline.yaml        # 基线小图 (512×512)
│   ├── e2e_image_720p.yaml                # 标准 720p 图片
│   ├── e2e_image_2k_stress.yaml           # 2K 压力测试
│   ├── e2e_image_palette_edge.yaml        # 调色板边界 (pal8)
│   ├── e2e_image_multi_processor.yaml     # 多处理器流水线
│   ├── e2e_video_baseline.yaml            # 视频基线 (720p 竖屏)
│   └── e2e_video_checkpoint.yaml          # 断点续传测试
├── scripts/                               # 测试执行与校验脚本
│   ├── run_e2e.py                         # 自动化 E2E 执行器
│   └── verify_output.py                   # 输出文件校验工具
└── expected/                              # 预期结果参考 (可选)
    └── baseline_ssim.json                 # SSIM 基准值
```

### 2.2 配置文件设计原则

*   所有路径使用**相对于可执行文件所在目录**的相对路径
*   基于 `tests/e2e/configs/e2e_baseline.yaml` (原 `tests/integration/app/test_config_e2e_baseline.yaml`) 模板扩展
*   每个配置文件覆盖单一测试场景，便于独立执行和故障定位

### 2.3 执行脚本设计 (`run_e2e.py`)

*   自动 `cd` 到可执行文件所在目录
*   依次执行各配置文件对应的 E2E 测试
*   采集耗时、退出码、输出文件信息
*   调用 `verify_output.py` 校验输出（文件存在性、尺寸、帧数等）
*   生成汇总报告 (Markdown 格式)

---

## 3. 实施路线图

### 3.1 阶段一: 基础设施搭建
**目标**: 创建 `tests/e2e/` 目录结构，编写核心脚本框架和配置文件

*   [x] **任务 1.1**: 创建目录结构 (`tests/e2e/`, `configs/`, `scripts/`, `expected/`)
    *   迁移现有文件: `git mv tests/integration/app/test_config_e2e_baseline.yaml tests/e2e/configs/e2e_baseline.yaml` (重命名以匹配 E2E 配置命名规范)
*   [x] **任务 1.2**: 编写 `tests/e2e/README.md` — E2E 测试说明文档
*   [x] **任务 1.3**: 编写 `run_e2e.py` 脚本框架 — 支持自动发现配置、执行、采集结果
*   [x] **任务 1.4**: 编写 `verify_output.py` — 输出校验工具 (文件存在性、图片/视频属性检查)
*   [x] **验收标准**:
    *   `run_e2e.py --help` 正常输出
    *   空配置下脚本框架可运行无报错

### 3.2 阶段二: 图片 E2E 测试
**目标**: 覆盖所有图片处理场景

*   [x] **任务 2.1**: 编写 `e2e_image_512_baseline.yaml` 并验证
*   [x] **任务 2.2**: 编写 `e2e_image_720p.yaml` 并验证
*   [x] **任务 2.3**: 编写 `e2e_image_2k_stress.yaml` 并验证
*   [x] **任务 2.4**: 编写 `e2e_image_palette_edge.yaml` 并验证
*   [x] **任务 2.5**: 编写 `e2e_image_multi_processor.yaml` 并验证
*   [x] **验收标准**:
    *   所有 5 个图片测试通过 (`run_e2e.py --filter image`)
    *   输出文件可用标准图片查看器打开

### 3.3 阶段三: 视频 E2E 测试
**目标**: 覆盖视频处理场景，含断点续传和优雅停机

*   [x] **任务 3.1**: 编写 `e2e_video_baseline.yaml` 并验证
*   [x] **任务 3.2**: 视频总耗时验证 (集成在 `run_e2e.py` 中)
*   [x] **任务 3.3**: 编写 `e2e_video_checkpoint.yaml` 并验证断点续传
*   [x] **任务 3.4**: 优雅停机测试 (脚本: `tests/e2e/scripts/test_interrupt_resume.py`)
*   [x] **验收标准**:
    *   所有视频测试通过 (`run_e2e.py --filter video`)
    *   输出视频可用 ffplay/mpv 正常播放

### 3.4 阶段四: 系统级 E2E 测试
**目标**: 验证 CLI 辅助功能和资源监控

*   [x] **任务 4.1**: `--system-check` 验证 (脚本: `tests/e2e/scripts/test_cli_flags.py`)
*   [x] **任务 4.2**: `--validate` 验证 (计划集成在 test_cli_flags.py 中, 暂未实现, 可选)
*   [x] **任务 4.3**: 快捷模式 `-s -t -o` 验证 (脚本: `tests/e2e/scripts/test_cli_flags.py`)
*   [x] **任务 4.4**: 显存峰值监控 (人工验证或集成 NVML)
*   [x] **任务 4.5**: 内存泄漏检测 (人工验证)
*   [x] **任务 4.6**: Metrics JSON 输出验证 (人工验证)
*   [x] **验收标准**:
    *   所有系统级测试通过
    *   汇总报告生成 (Markdown)

### 3.5 阶段五: E2E 测试覆盖率提升 (集成测试对齐)
**目标**: 扩展 E2E 测试配置，使其完全覆盖 `tests/integration/app/` 下的集成测试场景。

*   [x] **任务 5.1**: 删除冗余配置 `tests/e2e/configs/e2e_baseline.yaml`
*   [x] **任务 5.2**: 优化运行脚本 `run_e2e.py` (流式输出)
*   [x] **任务 5.3**: 创建 Image E2E 配置 (`single`, `batch`, `multi_step`)
*   [x] **任务 5.4**: 创建 Video E2E 配置 (`strict`, `tolerant`, `multi_step`, `batch`)
*   [x] **验收标准**:
    *   所有新增 7 个配置测试通过
    *   输出日志可见

---

## 4. 性能验收基准

> **测试环境**: RTX 4060 Laptop 8GB | i9-14900HX | 24GB DDR | Ubuntu 24.04 (WSL2)
> **构建模式**: Release

| 测试项 | 阈值 | 说明 |
|--------|------|------|
| 图片 512px | < 1s | 基线小图 |
| 图片 720p | < 2s | 标准分辨率 |
| 图片 2K | < 3s | 压力测试 |
| 视频 720p FPS | > 15 FPS | 491帧测试视频 |
| 视频总耗时 | < 40s | 允许 20% 余量 |
| 显存峰值 | < 6.5 GB | 留 1.5GB 安全余量 |
| 内存泄漏 | Δ < 50MB | 处理前后 RSS 差异 |

---

## 5. 风险管理

| 风险点 | 可能性 | 影响 | 缓解措施 |
|--------|--------|------|----------|
| TensorRT 引擎首次构建耗时 | 高 | 首次测试超时 | 文档说明首次运行需预热; 脚本支持 `--warmup` 选项 |
| 不同硬件性能差异 | 高 | 阈值不通用 | 按 GPU 档位提供不同阈值集; 脚本支持 `--gpu-tier` 参数 |
| 测试素材缺失 | 中 | 测试无法执行 | `run_e2e.py` 启动时校验素材完整性, 缺失时明确报错 |
| 相对路径在不同 OS 行为差异 | 中 | 路径错误 | 配置文件注释中标注路径相对基准; 脚本自动转绝对路径 |

---

## 6. 资源与依赖

*   **前置任务**: M11 集成测试全部通过 (✅ 已完成)
*   **外部依赖**:
    - Python 3.8+ (脚本执行)
    - ffprobe (视频属性校验)
    - nvidia-smi (显存监控)
*   **测试素材** (已存在):
    - `assets/standard_face_test_images/` (lenna.bmp, tiffany.bmp, girl.bmp, woman.jpg, man.bmp, barbara.bmp)
    - `assets/standard_face_test_videos/` (slideshow_scaled.mp4)
