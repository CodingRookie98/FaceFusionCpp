# 设计附录 (Design Appendix)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-DESIGN-APPENDIX-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-28

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-28 | AI Agent | 王辉 | 自 [design.md](./design.md) V3.3 拆分（文档治理 P3：单文档 ≤800 行红线），内容未变更。 |

> 所属总纲：[design.md](./design.md)

---

### A.3 标准测试素材 (Standard Test Assets)

为确保集成测试的一致性与可重复性，项目提供以下标准测试素材：

#### A.3.1 测试图片目录

**路径**: `assets/standard_face_test_images/`

| 文件名 | 分辨率 | 格式 | 像素格式 | 文件大小 | 用途 |
| :----- | :----- | :--- | :------- | :------- | :--- |
| `lenna.bmp` | 512×512 | BMP | bgr24 | 768 KB | **推荐 Source Face** (经典测试图) |
| `tiffany.bmp` | 512×512 | BMP | bgr24 | 768 KB | 标准 Target |
| `girl.bmp` | 720×576 | BMP | bgr24 | 1.2 MB | 720p Target |
| `barbara.bmp` | 720×576 | BMP | bgr24 | 1.2 MB | 720p Target |
| `man.bmp` | 1024×1024 | BMP | pal8 | 1.0 MB | 边界测试 (调色板格式) |
| `woman.jpg` | 1992×1120 | WebP* | yuv420p | 51 KB | 压力测试 (接近 2K) |

> **注意**: `woman.jpg` 实际为 WebP 格式 (扩展名伪装)，用于测试格式自动检测能力。

#### A.3.2 测试视频目录

**路径**: `assets/standard_face_test_videos/`

| 文件名 | 分辨率 | 编码 | 帧率 | 时长 | 帧数 | 文件大小 | 备注 |
| :----- | :----- | :--- | :--- | :--- | :--- | :------- | :--- |
| `slideshow_scaled.mp4` | 720×1280 | H.264 High + AAC | 30 FPS | 16.4s | 491 帧 | 2.6 MB | **主测试视频** (竖屏 9:16) |

#### A.3.3 硬件适配验收标准

**当前测试环境 (Reference Baseline)**:
- **CPU**: Intel Core i9-14900HX (16核32线程)
- **内存**: 24GB DDR
- **GPU**: NVIDIA GeForce RTX 4060 Laptop GPU
  - VRAM: 8GB GDDR6
  - CUDA 驱动: 591.86
  - 计算能力: 8.9 (Ada Lovelace 架构)
- **操作系统**: Linux (WSL2 / Native)

根据不同 GPU 配置，验收标准应进行相应调整：

> **⚠️ 构建模式**: 以下性能指标均基于 **Release 模式** 测试，Debug 模式数据不具参考价值。

**性能基准表 (Performance Benchmarks)**:

| 测试项 | RTX 4090 (24GB) | RTX 4060 (8GB)* | RTX 3060 (12GB) | GTX 1650 (4GB) | 说明 |
| :----- | :-------------- | :-------------- | :-------------- | :------------- | :--- |
| **图片处理 (512px)** | < 0.5s | < 1s | < 1.5s | < 3s | 基线小图 |
| **图片处理 (720p)** | < 1s | < 2s | < 2.5s | < 5s | 标准分辨率 |
| **图片处理 (2K)** | < 2s | < 3s | < 4s | < 10s | 压力测试 |
| **视频 FPS (720p)** | > 30 FPS | > 15 FPS | > 12 FPS | ≥ 5 FPS | 491帧测试视频 |
| **视频总耗时** | < 20s | < 40s | < 50s | < 120s | 允许 20% 余量 |
| **显存峰值** | < 18 GB | < 6.5 GB | < 10 GB | < 3.5 GB | 留安全余量 |

> **\*** 当前测试环境基准配置

**硬件适配策略**:

| 显存分级 | 推荐配置 | 适用场景 |
| :------- | :------- | :------- |
| **≥ 12GB** (旗舰级) | `memory_strategy: tolerant`<br>`max_queue_size: 30`<br>`execution_order: sequential` | 高分辨率 (4K) 实时处理<br>复杂流水线 (4+ Processors) |
| **8-12GB** (主流级) | `memory_strategy: strict`<br>`max_queue_size: 20`<br>`execution_order: sequential` | 2K 及以下标准处理<br>3 Processors 以内流水线 |
| **4-8GB** (入门级) | `memory_strategy: strict`<br>`max_queue_size: 15`<br>`execution_order: batch`<br>`batch_buffer_mode: disk` | 1080p 及以下<br>建议启用分段处理 |
| **< 4GB** (低端) | `memory_strategy: strict`<br>`max_queue_size: 10`<br>`execution_order: batch`<br>`segment_duration_seconds: 5` | 仅支持 720p 以下<br>强制分段 + 磁盘缓存 |

> **低显存适配建议 (≤ 4GB)**:
> - ✅ 必须使用 `memory_strategy: strict` (按需加载模型)
> - ✅ 降低 `max_queue_size` 至 10 (减少帧缓冲)
> - ✅ 启用 `execution_order: batch` + `batch_buffer_mode: disk` (降低峰值显存)
> - ✅ 设置 `segment_duration_seconds: 5` (强制分段处理长视频)
> - ⚠️ 测试分辨率不超过 720p
> - ⚠️ 避免同时启用超过 2 个 Processors

#### A.3.4 边界情况测试

| 测试项 | 素材 | 验证点 |
| :----- | :--- | :----- |
| **调色板图片** | `man.bmp` (pal8) | 自动转换为 RGB24 |
| **格式伪装** | `woman.jpg` (实际 WebP) | 正确解码 |
| **竖屏视频** | `slideshow_scaled.mp4` (9:16) | 纵横比正确处理 |
| **无人脸帧** | 视频中可能存在 | 透传 (Pass-through)，E403 警告日志 |

### A.5 数据格式选型 (Data Format Selection)

本项目同时依赖 `yaml-cpp` 和 `nlohmann-json` 两个库，根据使用场景采用不同的数据格式：

#### A.5.1 格式对比

| 维度             | **YAML**                     | **JSON/JSONC**               |
| :--------------- | :--------------------------- | :--------------------------- |
| **可读性**       | ✅ 优秀（缩进式，无括号噪音） | ⚠️ 中等（括号/逗号较多）      |
| **注释支持**     | ✅ 原生支持 `#`               | ✅ JSONC 支持 `//` 和 `/* */` |
| **多行字符串**   | ✅ 原生支持 `                 | ` 和 `>`                     | ❌ 需转义 |
| **解析性能**     | ⚠️ 较慢（语法复杂）           | ✅ 较快                       |
| **错误定位**     | ⚠️ 缩进错误定位较模糊         | ✅ 明确的行列定位             |
| **生态系统兼容** | ⚠️ 主要用于配置文件           | ✅ REST API / 前端集成标准    |

#### A.5.2 架构决策

| 用途                              | 格式 | 库              | 理由                     |
| :-------------------------------- | :--- | :-------------- | :----------------------- |
| **配置文件** (人编辑)             | YAML | `yaml-cpp`      | 可读性优秀，原生注释支持 |
| └─ `config/`                      |      |                 |                          |
|    └─ `app_config.yaml`           |      |                 |                          |
|    └─ `task_config.yaml`          |      |                 |                          |
| **数据输出** (机器生成/消费)      | JSON | `nlohmann-json` | 解析快，生态系统兼容性好 |
| └─ `metrics_{timestamp}.json`     |      |                 |                          |
| └─ `--system-check --json` 输出   |      |                 |                          |
| **API 响应** (Future Server Mode) | JSON | `nlohmann-json` | REST API 标准格式        |
| **FlatBuffers 辅助**              | JSON | `nlohmann-json` | Schema 验证与调试        |

> [!NOTE]
> **统一原则**: 配置文件统一使用 YAML，不提供 JSON 配置替代方案，避免维护成本和用户困惑。
