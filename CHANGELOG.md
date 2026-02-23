# Change Log

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
