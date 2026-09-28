# Vulkan 后端支持可行性评估报告

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-EVAL-VULKAN-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 评审中 (In Review)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-09-25

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-09-25 | AI Agent | 王辉 | 首次评估：针对 issue #1「add support vulkan」，完成推理层耦合度实测、ORT/替代引擎外部生态取证、候选方案对比、风险分级与 PoC 判定标准；确认 ORT 无官方 Vulkan EP，推荐接入官方 WebGPU plugin EP（Linux 即 Vulkan）作为唯一现实路径，并以两项上游约束作为立项门禁。 |

---

## 1. 评估背景与范围

**触发来源**：远端 issue [#1 add support vulkan](https://github.com/CodingRookie98/FaceFusionCpp/issues/1)（2026-09-24 由外部用户 Marijsoft 提交，正文为空）。

**核心诉求**：在非 NVIDIA 显卡（AMD / Intel）上获得 GPU 加速。

**评估目标**：
1. 判定"为项目新增 Vulkan 推理后端"的可行性与现实路径；
2. 量化现有推理层对 ONNX Runtime 的耦合度，界定改动面；
3. 输出候选方案对比、风险清单与 PoC 验收标准，为立项决策提供依据。

**评估方法**：
1. **代码实测**：对推理层进行静态取证，关键结论均附文件与行号；
2. **外部权威检索**：以官方文档、官方仓库 issue/release 为准，不采用二手转述；
3. **交叉核实**：对影响方案取舍的关键断言独立复核（证据等级标注见附录 A）。

**评估基准**：分支 `dev` @ `920a1fb`。

**环境实测约束**：本地为 WSL 环境且无 GPU 直通（NVML blocked），仅有 llvmpipe 软件 Vulkan 设备（1.4.318，含 compute queue）。因此本地仅能完成编译、功能与数值一致性验证，**性能与并发稳定性验收必须另备真实 AMD/Intel GPU 机器**。

---

## 2. 结论摘要

| 项 | 结论 |
| :--- | :--- |
| 诉求是否成立 | ✅ 成立：项目当前仅在 NVIDIA 上提供 GPU 加速，其他 GPU 全部回退 CPU EP |
| 是否可按字面实现 | ❌ 不可：ONNX Runtime 无官方 Vulkan EP，社区需求 #21917 自 2024-08 起 Open 且无实施计划 |
| 推荐路线 | **路线 A**：接入 ORT 官方 WebGPU plugin EP（Linux 上 Dawn 后端即 Vulkan） |
| 可行性评级 | **中**：技术路径成立，但受两项上游约束门禁 |
| 门禁 1（P0） | ORT #32561：Linux/Vulkan 下两个 session 并发 `Run()` 触发 SIGSEGV；项目默认 `max_concurrent_gpu_tasks = 2` 正命中该场景 |
| 门禁 2（P0） | WebGPU plugin EP 处于独立 `0.x` 版本线（最新 v0.4.0），算子覆盖仍在补齐 |
| 决策建议 | **先 PoC 后立项**（PoC 约 1 人日，判定点见 §7.1） |

---

## 3. 代码库现状实测

### 3.1 推理层与 ONNX Runtime 的耦合度

| 观测项 | 实测值 | 证据 |
| :--- | :--- | :--- |
| 推理引擎 | 唯一引擎 ONNX Runtime，无第二后端 | `src/foundation/ai/inference_session.ixx` |
| EP 枚举 | 仅 `CPU` / `CUDA` / `TensorRT` | `src/foundation/ai/inference_session.ixx:27-31` |
| EP 选择逻辑 | 硬编码优先级 TensorRT > CUDA > CPU，运行时探测可用性 | `src/foundation/ai/inference_session.cpp:37-62` |
| **接口抽象泄漏** | 公共 API 直接暴露 `Ort::Value` | `src/foundation/ai/inference_session.ixx:124` |
| **耦合规模** | **119 处 `Ort::` 调用，分布于 27 个文件** | 全仓静态检索 |
| 直接包含 ORT 头文件 | **28 个源文件** | 全仓静态检索 |
| 模型实现（domain 层） | 检测 / 关键点 / 识别 / 分类 / 交换 / 增强 / 遮挡 / 表情约 15 个实现类 | `src/domain/face/**`、`src/domain/frame/**` |
| 测试基建 | 15+ 单元测试通过 `preload_session` 注入返回 `Ort::Value` 的 mock | `tests/unit/domain/**` |
| 模型清单 | **28 个 ONNX**，含 GAN / Transformer 类（GFPGAN、CodeFormer、Real-HATGAN、Real-ESRGAN x8、LivePortrait ×3） | `assets/models_info.json` |
| 会话复用 | `InferenceSessionRegistry` + `SessionPool`（LRU + TTL） | `src/foundation/ai/session_pool.ixx` |

**推论**：更换推理引擎（ncnn / MNN / IREE）需要重写 119 处调用点、重建 15+ mock 测试并逐模型转换验证；而"新增一个 Execution Provider"只需改动会话创建层与配置层。**这是路线取舍的决定性事实。**

### 3.2 构建与配置现状

| 观测项 | 实测 | 证据 |
| :--- | :--- | :--- |
| ORT 获取方式 | CMake 硬编码下载 `onnxruntime-{linux,win}-x64-gpu-1.24.1` | `cmake/onnxruntime-gpu.cmake:9,12` |
| 实测包内容 | 仅 `providers_cuda` / `providers_tensorrt` / `providers_shared` 三个 provider，**无 Vulkan provider** | 本地解压 `build/downloads/onnxruntime-linux-x64-gpu-1.24.1.tgz` |
| `FACEFUSION_ENABLE_GPU` | **空转开关**：`build.py` 传入 CMake，全仓库无消费点 | `build.py:467` |
| `inference.default_providers` | **未接线**：YAML 解析入结构体后从未映射为 `ExecutionProvider`，实际全走自动探测 | `src/app/config/parser/config_parser.cpp:330-332`、`src/app/config/app_config.ixx:39` |
| 文档与实现不一致 | 用户 FAQ 建议"确保 tensorrt 优先级最高"，该配置当前不生效 | `docs/user/zh/faq.md:89` |
| 并发参数 | `max_concurrent_gpu_tasks = 2`、`worker_thread_count = 4` | `src/domain/pipeline/pipeline_types.ixx:71,73` |
| 现存 Vulkan 痕迹 | 全仓库 **0 处** | 全仓静态检索 |

> ⚠️ 上述两项"未接线"意味着：即使仅在 `app.yaml` 中追加 `webgpu` 也不会生效，**配置层必须先接线**（见 §7.2 阶段 1）。

### 3.3 并发形态（决定门禁 1 的影响面）

帧级流水线以 4 个 worker 并发处理帧，由 `max_concurrent_gpu_tasks` 信号量限制为 2 并发（`src/domain/pipeline/impl/pipeline_impl.ixx:39-40,116`），且 `runner_video.cpp` / `runner_image.cpp` 共 5 处显式设为 2。

同一帧顺序经过多个处理阶段（检测 → 交换 → 增强等），不同帧可并发处于不同阶段 → **存在两个不同 `InferenceSession` 并发 `Run()` 的真实场景**，即门禁 1 的复现条件。

---

## 4. 外部生态调研

### 4.1 ONNX Runtime 官方 EP 现状

| 事实 | 证据等级 | 来源 |
| :--- | :--- | :--- |
| 官方 EP 列表中**没有 Vulkan EP** | 🔎 官方文档 | [ORT Execution Providers](https://onnxruntime.ai/docs/execution-providers/) |
| 社区 Vulkan EP 需求 #21917 自 2024-08-29 起 Open，54 👍，无实施计划 | 🔎 官方仓库 | [microsoft/onnxruntime#21917](https://github.com/microsoft/onnxruntime/issues/21917) |
| ORT 核心最新版 1.30.0（2026-09-10），本项目使用 1.24.1 | 🔎 官方 release | [onnxruntime releases](https://github.com/microsoft/onnxruntime/releases) |

### 4.2 可行路径：ORT WebGPU plugin EP

| 事实 | 证据等级 | 来源 |
| :--- | :--- | :--- |
| WebGPU EP 为**官方插件 EP**，基于 Dawn，按平台分发到原生图形 API | 🔎 官方文档 | [WebGPU EP](https://onnxruntime.ai/docs/execution-providers/WebGPU-ExecutionProvider.html) |
| 平台→后端映射：**Linux/Android → Vulkan**；Windows → D3D12 + Vulkan；macOS/iOS → Metal | 🔎 官方文档 | 同上 |
| 定位为跨厂商、跨平台，**终端用户无需安装厂商 SDK** | 🔎 官方文档 | 同上 |
| C/C++ 接入：消费 `onnxruntime_providers_webgpu.{dll,so,dylib}`，经 `RegisterExecutionProviderLibrary` + `OrtEpDevice` 附加到 `SessionOptions` | 🔎 官方文档 | 同上 |
| **本项目 ORT 1.24.1 头文件已含全套插件 EP API**（`RegisterExecutionProviderLibrary`、`GetEpDevices`、`OrtEpDevice`，且 `onnxruntime_ep_c_api.h` 存在） | ✅ 本机实测 | 解压 `onnxruntime-linux-x64-gpu-1.24.1.tgz` 检索头文件 |
| 插件 EP 走**独立版本线**：v0.2.1 / v0.3.0 / v0.4.0（最新 2026-09-22），仍为 `0.x` | 🔎 官方 release | [plugin-ep-webgpu/v0.4.0](https://github.com/microsoft/onnxruntime/releases/tag/plugin-ep-webgpu%2Fv0.4.0) |
| 算子覆盖逐版扩展（v0.3.0 增 PRelu/HardSwish/GRU/int64 系列；v0.4.0 优化 Conv/MatMul 融合） | 🔎 官方 release | 同上 |
| ⚠️ **已知崩溃**：Linux/Vulkan 后端下两个 `InferenceSession` 并发 `Run()` 触发 SIGSEGV（2026-09-11 提交，仍 Open） | 🔎 官方 issue | [microsoft/onnxruntime#32561](https://github.com/microsoft/onnxruntime/issues/32561) |

### 4.3 备选引擎方案（换引擎路线）

| 方案 | 关键事实 | 证据等级 |
| :--- | :--- | :--- |
| **ncnn** | Vulkan 后端成熟、BSD-3、无第三方运行时依赖；但**仅支持静态 shape**；**DeformableConv2d 无 Vulkan 实现**（`src/layer/deformableconv2d.cpp` 存在，`src/layer/vulkan/deformableconv2d_vulkan.cpp` 不存在）；无 inswapper 移植先例 | ✅ 本机 `gh api` 直查文件（DCN 结论）；🔎 官方仓库/文档（其余） |
| **MNN** | 有 Vulkan 后端与 MNNConvert（支持 ONNX）；但桌面 CV 场景可用性证据稀少，官方重心在 LLM / NPU | 🔎 官方仓库 + 社区 issue |
| **IREE** | Vulkan HAL 需 Vulkan 1.3 及多项扩展；ONNX 前端质量与量化支持偏弱；部署链最长 | 🔎 官方指南 + 官方 issue |
| **ggml / llama.cpp** | Vulkan 后端面向 LLM/Transformer，无 ONNX 导入路径，CV 算子缺失 | 🔎 官方仓库/PR |

### 4.4 已排除方案

| 方案 | 排除理由 | 证据等级 |
| :--- | :--- | :--- |
| **DirectML EP** | 仅 Windows；官方标注 **sustained engineering**（新功能转向 WinML）；opset ≤ 20，且**不支持 GridSample 5d 与 DeformConv** | 🔎 官方文档（本机复核原文） |
| **OpenVINO GPU** | 仅枚举 Intel GPU，无法覆盖 AMD | 🔎 官方文档 |
| **ROCm / MIGraphX EP** | Linux + ROCm 限定；ROCm EP 据调研已标记 deprecated（**未独立核实**） | ⚠️ 未独立核实 |
| **ZLUDA 等 CUDA 兼容层** | 实验性注入层，且 ORT CUDA EP 依赖 cuDNN/cuBLAS，风险不可控 | 🔎 仓库活跃度（风险判定为推断） |
| **第三方树外 Vulkan EP** | 单人项目、无生产验证 | 🔎 仓库规模 |

---

## 5. 候选方案对比

| 维度 | A. ORT WebGPU plugin EP | B. 切换 ncnn / MNN | C. 维持 NVIDIA-only |
| :--- | :--- | :--- | :--- |
| 改动面 | **小**：会话创建 + EP 枚举 + 配置接线 + 构建分发 | **极大**：119 处调用点重写 + 28 模型转换验证 + 15+ mock 测试重建 | 零 |
| 非 NVIDIA 覆盖 | Windows + Linux，AMD / Intel / NVIDIA 通用 | 理论覆盖全 Vulkan 平台 | 无 |
| 模型兼容性 | 复用现有 .onnx，无需转换 | 需转换；DCN / 动态 shape / GAN 保真度存疑 | 不适用 |
| 主要风险 | 插件 0.x 版本线；并发崩溃 #32561；算子回退 CPU | 工期数量级差异；转换失败；精度回归 | issue #1 诉求不满足 |
| 许可证影响 | MIT（ORT） | BSD-3 / Apache-2.0 | 不适用 |
| 结论 | ✅ **唯一推荐（PoC 门禁）** | ❌ 不推荐 | 基线（零改动） |

---

## 6. 风险清单

| ID | 风险 | 等级 | 说明与缓解 |
| :--- | :--- | :--- | :--- |
| R-1 | Linux/Vulkan 并发 `Run()` SIGSEGV（ORT #32561） | **P0** | 与项目 2 并发闸门正命中；缓解：PoC 判定点 ② 优先复现，若命中则临时降为 1 并发并向上游报障 |
| R-2 | 插件 EP `0.x` API 与版本兼容性 | **P0** | 项目 ORT 1.24.1 vs 插件 v0.4.0（核心已 1.30.0）；缓解：判定点 ① 前先验证注册与设备枚举成功 |
| R-3 | 算子覆盖不足导致节点回退 CPU | P1 | 回退为静默 split，形成性能悬崖；缓解：判定点 ① 统计 GPU 节点占比，重点核查 GAN/Transformer 模型 |
| R-4 | fp16 模型精度（`inswapper_128_fp16` 等） | P1 | 缓解：判定点 ③ 逐模型误差门限回归（嵌入余弦相似度 / PSNR） |
| R-5 | Windows 上 Dawn 默认走 D3D12 而非 Vulkan | P2 | 与用户字面预期存在落差，需向 issue 作者说明 |
| R-6 | 显存策略与 WebGPU 上下文行为未知 | P2 | `memory_strategy: strict` + 会话池 LRU/TTL 在 WebGPU 上无先例；PoC 观测显存曲线 |
| R-7 | 本地无法做性能验收 | P1（流程） | 需真实 AMD/Intel GPU 机器 |
| R-8 | `default_providers` 与 `--enable-gpu` 未接线 | P1（前置） | 配置接线是接入的前置步骤，并需同步修正用户文档 |

---

## 7. 推荐路线与 PoC 计划

### 7.1 PoC 三个判定点（约 1 人日）

| # | 判定点 | 方法 | 通过标准 |
| :--- | :--- | :--- | :--- |
| ① | **EP 可用性与节点覆盖** | 注册 WebGPU plugin EP，对默认管线 5 个模型（yoloface / arcface / inswapper / gfpgan / real_esrgan）读取 ORT 分区日志，统计回退 CPU 的节点 | 会话创建成功，且主要计算节点（Conv/MatMul/GAN 主干）落在 WebGPU |
| ② | **并发稳定性** | 在 `max_concurrent_gpu_tasks = 2` 下刻意并发两个不同 session 的 `Run()` | 无 SIGSEGV；若复现则评估降为 1 并发的性能代价 |
| ③ | **数值一致性** | 与 CPU EP 逐模型比对 | 人脸嵌入余弦相似度、GAN 输出 PSNR 在可接受门限内（门限随 PoC 结果标定） |

### 7.2 若 PoC 通过的实施阶段（预估）

| 阶段 | 内容 | 预估 |
| :--- | :--- | :--- |
| 0 | PoC（判定点 ①–③） | 1 人日 |
| 1 | 接入：`ExecutionProvider` 增加 WebGpu；会话创建注册插件 EP；**配置层接线**（`default_providers` → 枚举映射）；构建/打包分发 `onnxruntime_providers_webgpu.*` | 3–5 人日 |
| 2 | 回归：28 模型逐一出图校验；并发场景压测；文档同步（README / configuration_guide / faq 的未接线项修正） | 2–3 人日 |
| 3 | 真实 AMD/Intel GPU 机器上的性能与稳定性验收 | 依赖硬件到位 |

### 7.3 立项流程要求

本变更涉及公共契约（`ExecutionProvider` 枚举、配置 schema、构建产物），按项目规范须走 `goal-loop` 重型通道：先出计划文档（`writing-plans` 命名规范，归档至 `docs/project/plans/`），经批准后按 TDD 实施，交付前执行 `dual-round-review` 并取得 Zero Blockers 裁决。

---

## 8. 待决问题与显式默认假设

**已在 issue #1 提出、等待作者回复的澄清项**（2026-09-25 已回帖 [issuecomment-5831932572](https://github.com/CodingRookie98/FaceFusionCpp/issues/1#issuecomment-5831932572)）：

1. **目标范围**：替换现有后端，还是新增一个可切换 provider？
2. **平台与 GPU**：Windows / Linux？具体 GPU 型号？Windows 上是否处于 D3D12 不可用或更慢的场景？
3. **验收标准**：硬件上功能正确即可，还是有明确性能目标？

**在作者回复前，PoC 可采用的显式默认假设**（需项目所有者确认后生效）：

| 编号 | 默认假设 | 依据 |
| :--- | :--- | :--- |
| A-1 | 目标为**新增可切换 provider**，不替换现有 TensorRT/CUDA 路径 | 最小侵入，且保留 NVIDIA 现有性能 |
| A-2 | 覆盖平台为 **Windows + Linux 双端** | issue 未限定平台，仓库双端发布 |
| A-3 | 验收 = 功能正确 + 数值达标 + 不劣于 CPU 基线 | issue 未给出性能指标 |

---

## 附录 A：证据等级说明

| 标记 | 含义 |
| :--- | :--- |
| ✅ 本机实测 | 在本地环境直接执行命令 / 解压 / 接口检索得到的结论 |
| 🔎 官方来源检索 | 来自官方文档、官方仓库 issue 或 release 的结论 |
| ⚠️ 未独立核实 | 来自二手信息，未取得一手证据，仅作风险提示 |

## 附录 B：本地环境实测记录

| 项 | 实测结果 |
| :--- | :--- |
| GPU 直通 | `nvidia-smi` 报 "GPU access blocked by the operating system"（WSL 无直通） |
| 显示设备 | `lspci` 仅见 "Microsoft Corporation Basic Render Driver" |
| Vulkan 设备 | 自建探针枚举：`llvmpipe (LLVM 20.1.2, 256 bits)`，API 1.4.318，type=CPU(4)，含 compute queue |
| Vulkan 工具链 | loader 1.3.275、`vulkan/vulkan.h`、`glslc`、`spirv-val` 齐备 |
| 既有 ORT 包 | `build/downloads/onnxruntime-linux-x64-gpu-1.24.1.tgz`（205 MB），provider 仅 cuda / tensorrt / shared |
| 结论 | 本地可做功能与数值冒烟，**不可做性能与并发稳定性验收** |

---

## 参考文档

- [design.md](../architecture/design.md) —— 系统设计与推理基础设施章节
- [layers.md](../architecture/layers.md) —— 分层结构与 foundation/ai 职责边界
- [architecture-performance-review.md](architecture-performance-review.md) —— 推理架构既有性能评估（含会话层/池层问题清单）
- [C++_evaluation_template.md](../../../archived/C++_evaluation_template.md) —— 评估文档模板（已废止归档）
