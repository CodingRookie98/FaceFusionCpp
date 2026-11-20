# Change Log

## [v0.34.2]

### English

#### Added

- **Web UI**: Rebuilt the studio as "Deep Studio Workbench V2" — dynamic multi-processor pipeline editor, task queue panel (priority promote/demote/cancel), omni-perception canvas preview, multi-face selection with batch actions, and split compare/result views.
- **Web API**: Task creation accepts structured `pipeline_steps`; new `/api/preview` endpoint; per-task output isolation and instant preview rendering.
- **Web runtime**: Task snapshots are persisted and restored after restart (`web.persist_dir`); per-task execution timeout (`web.max_execution_seconds`); WebSocket error/status frames dispatched through the event loop.
- **Configuration**: `inference.default_providers` is now wired — sessions follow the configured provider priority (default TensorRT > CUDA > CPU) with availability filtering and fallback; `inference.device_id` and `inference.engine_cache.enable/path` are applied.
- **Configuration**: `engine_cache.max_entries` / `idle_timeout_seconds` now configure the session pool; default capacity raised from 3 to 10 so a single task's six sessions are not evicted.
- **Build**: `build.py` builds the Web frontend as part of `--action build`/`package`; `--action dev` launches backend + Vite dev server together.
- **Observability**: startup banner, configuration summary, hardware report, and a `Session Pool: capacity=..., ttl=...s` line.

#### Changed

- **Performance**: session pool capacity 3→10, domain services reused across videos, InSwapper embedding transform cached, face-cache toggle for the video path, GPU concurrency gate, session-key model fingerprinting for hot reload, TTL lazy cleanup, session factory moved outside the pool lock, strict-mode queue cap 4→16, checkpoint saving throttled to every 100 frames.
- **Performance**: face enhancer skips the full-frame clone at 100% blend, and the in-place `addWeighted` UB is fixed (blend < 100 previously produced unchanged output).
- **Build**: `--enable-gpu` is now an explicit no-op — GPU support is always compiled in and provider choice happens at runtime.
- **Docs**: knowledge base migrated to the Diátaxis six-quadrant topology with CI doc gates (links / revision window / health); workflow documents slimmed (methodology delegated to skills, contracts stay in-repo); compiler-selection ADR renumbered as MADR 3.0 `0001-compiler-selection.md` with a weighted decision matrix.
- **Tests**: task-manager test helpers wait for a terminal status instead of "not queued", removing a long-standing flake.

#### Fixed

- **Web**: WebSocket flakiness, task teardown segfault, non-ASCII upload filenames, result media serving, preview fallback after reload, blob URL sanitization, split-comparison synchronization.
- **Build**: metadata sync script could clobber README code-block comments and rewrite the CMake project name; it now rewrites only first-match headings and leaves `project(...)` alone.
- **Build**: E2E runner referenced the old binary name `FaceFusionCpp` instead of `ffc`.

#### Removed

- **Web UI**: legacy single-page studio layout (replaced by the V2 workbench).

### 中文

#### 新增

- **Web UI**：重构为「Deep Studio Workbench V2」——动态多处理器流水线编辑器、任务队列面板（优先级升降/取消）、全感知画布预览、多脸选择与批量操作、分屏对比与结果视图。
- **Web API**：任务创建支持结构化 `pipeline_steps`；新增 `/api/preview`；按任务隔离输出目录并即时渲染预览。
- **Web 运行时**：任务快照落盘并在重启后恢复（`web.persist_dir`）；单任务执行超时（`web.max_execution_seconds`）；WebSocket 错误/状态帧经事件循环投递。
- **配置**：`inference.default_providers` 已接线——会话按配置的优先级（默认 TensorRT > CUDA > CPU）选择后端，并自动做可用性过滤与降级；`inference.device_id` 与 `inference.engine_cache.enable/path` 同步生效。
- **配置**：`engine_cache.max_entries` / `idle_timeout_seconds` 现用于配置会话池；默认容量由 3 提升到 10，避免单任务六个会话互相驱逐。
- **构建**：`build.py` 在 `--action build`/`package` 中构建 Web 前端；`--action dev` 一键同时启动后端与 Vite 开发服务器。
- **可观测性**：启动 Banner、配置摘要、硬件报告，以及 `Session Pool: capacity=..., ttl=...s` 启动日志。

#### 变更

- **性能**：会话池容量 3→10、跨视频复用领域服务、InSwapper 变换缓存、视频路径人脸缓存开关、GPU 并发闸门、会话 Key 加入模型指纹（热更新生效）、TTL 惰性清理、会话创建移出池锁、Strict 模式队列上限 4→16、checkpoint 每 100 帧落盘。
- **性能**：人脸增强在 100% 混合时跳过整帧克隆，并修复 `addWeighted` in-place UB（此前 blend < 100 输出无变化）。
- **构建**：`--enable-gpu` 改为显式 no-op——GPU 支持始终编译，后端选择在运行期由配置决定。
- **文档**：知识库迁移至 Diátaxis 六象限并接入 CI 门禁（断链 / 修订窗口 / 健康度）；流程文档瘦身（方法层让位技能库、契约层留在仓库）；编译器选型 ADR 重编号为 MADR 3.0 `0001-compiler-selection.md` 并补加权决策矩阵。
- **测试**：任务管理器测试助手改为等待终态而非「离开 Queued」，消除长期 flaky。

#### 修复

- **Web**：WebSocket 抖动、任务销毁段错误、非 ASCII 上传文件名、结果媒体文件服务、刷新后预览回退、blob URL 消毒、分屏对比同步。
- **构建**：元数据同步脚本会误改 README 代码块注释与 CMake 工程名；现仅改写首个标题匹配且不再触碰 `project(...)`。
- **构建**：E2E 执行器使用了旧二进制名 `FaceFusionCpp`，已改为 `ffc`。

#### 移除

- **Web UI**：旧版单页 Studio 布局（由 V2 工作台取代）。

## [v0.34.1]

### English

#### Added

- **Testing**: Added `video_encoder: "hevc_nvenc"` configuration to multi-step E2E tests.
- **Documentation**: Added comprehensive beginner explanations with default values and supported model lists for pipeline processors.

#### Changed

- **CI/CD**: Simplified GitHub Releases naming structure in workflows.
- **Documentation**: Consolidated branching strategy tracking (adopted standard Git Flow) and refined configuration/troubleshooting instructions.

#### Fixed

- **Documentation**: Resolved markdown linting warnings (spacing, indentation) across both English and translated user documents.
- **Video Processing**: Fixed an issue where output `video_encoder` and `video_quality` configurations were silently ignored by the video writer.

#### Removed

- **Build System**: Cleaned up redundant `cpack_all` custom target in `CMakeLists.txt` (natively handled by CPack).
- **Repository**: Removed obsolete `.specify` directory from Git tracking.

### 中文

#### 新增

- **测试**：在多步 E2E 测试配置中增加了 `hevc_nvenc` 硬件视频编码器支持。
- **文档**：在用户指南中新增了详细的新手说明、默认值以及各管线处理器所支持的模型列表。

#### 变更

- **CI/CD**：简化了 GitHub Actions 中的 Release 版本命名格式。
- **文档**：统一了分支策略记录（采用标准的 Git Flow 模式），并优化了配置与故障排除指南。

#### 修复

- **文档**：修复了中英文用户手册及配置指南中的 Markdown 语法警告（如间距和代码块缩进问题）。
- **视频处理**：修复了输出配置中的 `video_encoder`（视频编码器）和 `video_quality`（视频质量）参数被视频写入器忽略的问题。

#### 移除

- **构建系统**：移除了 `CMakeLists.txt` 中冗余的 `cpack_all` 目标（已由原生 CPack 完全接管）。
- **代码库**：清理了代码库，将废弃的 `.specify` 目录移出 Git 版本控制。

## [v0.34.0]

### English

#### Added

- **CI/CD Pipeline**: Implemented full GitHub Actions infrastructure for automated builds, testing, and release drafting.
- **E2E Testing Framework**: Added comprehensive end-to-end testing suite with automated runners and video/image verification.
- **CLI Enhancements**: Introduced startup banners, system health checks (CUDA/cuDNN/TensorRT), and structured task configuration validation.
- **Model Management**: Implemented `ModelRepository` with dynamic path resolution and lifecycle strategies.
- **Performance Monitoring**: Added VRAM and memory usage monitoring during integration and E2E tests.
- **Batch Processing**: Supported batch image processing and optimized runner architecture for large-scale tasks.
- **Audio Support**: Implemented audio muxing for video processing.

#### Changed

- **Architecture Migration**: Completed migration of core modules to C++20 Modules for better isolation and build performance.
- **Build System**: Replaced legacy PowerShell scripts with a unified `build.py` Python script for cross-platform consistency.
- **Toolchain**: Migrated to Clang 21 / LLD 21 and optimized CMake presets.
- **Face Processing**: Refactored `FaceAnalyser` and `FaceStore` (added LRU cache and improved thread safety).
- **Dependency Management**: Optimized vcpkg binary caching and improved dependency resolution for Linux/Windows.
- **Documentation**: Overhauled entire documentation suite, including developer guides, architecture maps, and user manuals.

#### Fixed

- **Stability**: Resolved TensorRT engine cache filesystem errors and teardown crashes.
- **Platform Compatibility**: Fixed numerous MSVC-specific build failures related to modules and headers.
- **Pipeline Issues**: Corrected frame dropping in producer loops and resolved progress bar flickering/wrapping.
- **Resource Management**: Fixed race conditions in `ModelRepository` and improved resource cleanup order.

#### Removed

- Depleted legacy PowerShell/Bash build scripts.
- Removed obsolete `specs/` and old planning documents.
- Cleaned up redundant local GitHub actions in favor of composite actions.

### 中文

#### 新增

- **CI/CD 流水线**：实现了完整的 GitHub Actions 基础设施，用于自动化构建、测试和发布草稿。
- **E2E 测试框架**：添加了端到端测试套件，包含自动化运行器及视频/图像验证功能。
- **CLI 增强**：引入了启动横幅、系统健康检查（CUDA/cuDNN/TensorRT）以及结构化的任务配置校验。
- **模型管理**：实现了 `ModelRepository`，支持动态路径解析和生命周期管理策略。
- **性能监控**：在集成测试和 E2E 测试中增加了显存（VRAM）和内存使用情况监控。
- **批处理支持**：支持批量图像处理，并针对大规模任务优化了运行器架构。
- **音频支持**：实现了视频处理中的音频混合功能。

#### 变更

- **架构迁移**：完成了核心模块向 C++20 Modules 的迁移，提升了代码隔离度和构建性能。
- **构建系统**：使用统一的 `build.py` Python 脚本替换了旧的 PowerShell 脚本，确保跨平台一致性。
- **工具链**：迁移至 Clang 21 / LLD 21，并优化了 CMake 预设配置。
- **人脸处理**：重构了 `FaceAnalyser` 和 `FaceStore`（增加了 LRU 缓存并提升了线程安全性）。
- **依赖管理**：优化了 vcpkg 二进制缓存，并改进了 Linux/Windows 下的依赖解析。
- **文档体系**：全面修订了文档库，包括开发指南、架构图和用户手册。

#### 修复

- **稳定性**：解决了 TensorRT 引擎缓存文件系统错误以及退出时的崩溃问题。
- **平台兼容性**：修复了大量 MSVC 环境下与模块和头文件相关的构建失败问题。
- **流水线问题**：修正了生产者循环中的掉帧问题，并解决了进度条闪烁或换行显示的问题。
- **资源管理**：修复了 `ModelRepository` 中的竞态条件，并改进了资源清理顺序。

#### 移除

- **清理旧脚本**：移除了过时的 PowerShell/Bash 构建脚本。
- **删除陈旧文档**：删除了废弃的 `specs/` 目录和旧的规划文档。
- **精简 Actions**：清理了冗余的本地 GitHub Actions，改用复合 Action (Composite Actions)。
