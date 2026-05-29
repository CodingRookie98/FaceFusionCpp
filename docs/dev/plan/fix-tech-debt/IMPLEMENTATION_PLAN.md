# 实施计划：修复技术债务与代码坏味道

## 概览

基于 `docs/dev/zh/evaluation/C++_evaluation_code_smells_and_tech_debt.md` 验证结果，按优先级分批修复已确认的技术债务问题。

## 阶段一：P1 快速修复（改动小、收益高）

### 子任务 1: concurrent_queue.ixx 现代化
**目标**: 升级 lock_guard → scoped_lock，补齐 Rule of 5
**修改文件**: `src/foundation/infrastructure/concurrent_queue.ixx`
**具体改动**:
- 6 处 `std::lock_guard<std::mutex>` → `std::scoped_lock<std::mutex>`
- 添加显式移动删除: `ConcurrentQueue(ConcurrentQueue&&) = delete;` 和 `operator=(ConcurrentQueue&&) = delete;`
**测试**: 现有 `concurrent_queue_test.cpp` 应全部通过（编译验证为主）
**状态**: 已完成

### 子任务 2: process.ixx 接口修正
**目标**: 添加 `explicit` 和 `[[nodiscard]]`
**修改文件**: `src/foundation/infrastructure/process.ixx`
**具体改动**:
- 2 个 Process 构造函数添加 `explicit`
- 3 个查询函数添加 `[[nodiscard]]`: `get_id()`, `get_exit_status()`, `try_get_exit_status()`
**测试**: 现有 process 相关测试应全部通过
**状态**: 已完成

### 子任务 3: network.ixx 类型安全
**目标**: 将 `long` 替换为 `int64_t`
**修改文件**: `src/foundation/infrastructure/network.ixx`, `src/foundation/infrastructure/network.cpp`
**具体改动**:
- 9 处 `long` → `std::int64_t`
- 需要添加 `#include <cstdint>`（如尚未包含）
**测试**: 现有 network 相关测试应全部通过
**状态**: 已完成

### 子任务 4: 剩余枚举底层类型补全
**目标**: 为 3 个未指定底层类型的枚举添加定宽类型
**修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/app/cli/system_check.ixx`, `src/foundation/infrastructure/process.ixx`
**具体改动**:
- `ShutdownState` → `enum class ShutdownState : std::uint8_t`
- `CheckStatus` → `enum class CheckStatus : std::uint8_t`
- `ShowWindow` → `enum class ShowWindow : std::uint8_t`
**测试**: 编译验证 + 现有相关测试
**状态**: 已完成

### 子任务 5: Rule of 5 补齐
**目标**: 为 shutdown_handler.ixx 和 checkpoint_manager.ixx 补齐移动语义声明
**修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/services/pipeline/checkpoint_manager.ixx`
**具体改动**:
- shutdown_handler.ixx: 添加 `ShutdownHandler(ShutdownHandler&&) = delete;` 和 `operator=(ShutdownHandler&&) = delete;`
- checkpoint_manager.ixx: 添加 `CheckpointManager(CheckpointManager&&) = delete;` 和 `operator=(CheckpointManager&&) = delete;`
**测试**: 编译验证
**状态**: 已完成

### 子任务 6: make_shared/make_unique 替换
**目标**: 消除裸 `new` 的使用
**修改文件**: `src/foundation/infrastructure/logger.cpp`, `src/foundation/ai/inference_session_registry.cpp`, `src/domain/pipeline/pipeline_adapters.cpp`, `src/foundation/infrastructure/process.cpp`
**具体改动**:
- `std::shared_ptr<T>(new T())` → `std::make_shared<T>()` (5 处)
- `new char[buffer_size]` → `std::make_unique<char[]>(buffer_size)` (3 处)
- 测试文件中的 `new float[]` (1 处) 也一并修复
**测试**: 编译验证 + 现有相关测试
**状态**: 已完成

## 阶段二：P2 架构重构（需测试覆盖验证）

### 子任务 7: shutdown_handler.ixx PIMPL 封装
**目标**: 将平台相关实现和私有成员隐藏到 .cpp
**修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/services/pipeline/shutdown_handler.cpp`
**具体改动**:
- 移除 `.ixx` 中的 `#include <Windows.h>` 和 `#ifdef _WIN32`
- 将所有 `private` 成员移入 `.cpp` 的 PIMPL 或匿名命名空间
- `.ixx` 仅保留 public 接口 + 前向声明
**测试**: shutdown_handler 相关测试 + 编译验证
**状态**: 未开始

### 子任务 8: FFmpeg RAII 封装
**目标**: 用 unique_ptr+custom deleter 替代手动 cleanup()
**修改文件**: `src/foundation/media/ffmpeg_reader.cpp`, `src/foundation/media/ffmpeg_writer.cpp`
**具体改动**:
- 为 AVFormatContext, AVCodecContext, SwsContext 等定义 RAII wrapper 类型别名
- 替换 28 处手动 `cleanup()` 调用
- 确保析构路径安全
**测试**: ffmpeg_reader/ffmpeg_writer 相关测试 + 集成测试
**状态**: 未开始

## 不修复项（已过时）

以下问题经验证已不成立，无需处理：
- ~~`sort_targets_by_type` 值传递~~ — 已使用 `const &`
- ~~`thread_pool.ixx` 缺少 `std::forward`~~ — 已正确使用

## P3 暂缓项

- 全局 CamelCase → snake_case 重命名（ffmpeg.ixx, config_merger.ixx）— 影响面广，需单独排期
