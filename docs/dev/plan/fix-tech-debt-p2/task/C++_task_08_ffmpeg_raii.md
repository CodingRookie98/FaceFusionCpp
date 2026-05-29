# 子任务 8: FFmpeg RAII 封装

## 基本信息
- **所属计划**: fix-tech-debt-p2
- **优先级**: P2
- **修改文件**: `src/foundation/media/ffmpeg_reader.cpp`, `src/foundation/media/ffmpeg_writer.cpp`
- **状态**: 待开始

## 目标
用 `unique_ptr` + 自定义 deleter 替代手动 `cleanup()` 调用，实现 RAII 资源管理。

## 研究发现

### 当前问题
1. FFmpeg 资源（AVFormatContext, AVCodecContext 等）需要手动释放
2. 多处 `cleanup()` 调用容易遗漏，导致资源泄漏
3. 异常路径下资源可能未正确释放

### RAII 模式优势
1. **自动释放**：析构函数自动调用，无需手动管理
2. **异常安全**：栈展开时自动释放资源
3. **代码简洁**：消除重复的 cleanup 代码

## 具体改动

### 1. 定义 RAII Wrapper 类型别名
```cpp
// 在 ffmpeg_common.ixx 或 ffmpeg.ixx 中
namespace ffmpeg {

// 自定义 Deleter
struct AVFormatContextDeleter {
    void operator()(AVFormatContext* ctx) {
        if (ctx) avformat_close_input(&ctx);
    }
};

struct AVCodecContextDeleter {
    void operator()(AVCodecContext* ctx) {
        if (ctx) avcodec_free_context(&ctx);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* ctx) {
        if (ctx) sws_freeContext(ctx);
    }
};

// 类型别名
using AVFormatContextPtr = std::unique_ptr<AVFormatContext, AVFormatContextDeleter>;
using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

} // namespace ffmpeg
```

### 2. 替换手动管理为 RAII
```cpp
// 旧代码
AVFormatContext* format_ctx = nullptr;
avformat_open_input(&format_ctx, filename, nullptr, nullptr);
// ... 使用 ...
avformat_close_input(&format_ctx);  // 手动释放

// 新代码
ffmpeg::AVFormatContextPtr format_ctx;
avformat_open_input(&format_ctx, filename, nullptr, nullptr);
// ... 使用 ...
// 自动释放，无需手动调用
```

### 3. 处理特殊情况
```cpp
// 对于需要延迟释放的场景
ffmpeg::AVFormatContextPtr format_ctx;
// ... 使用 ...
format_ctx.release();  // 显式释放所有权
// 或
format_ctx.reset();  // 重置并释放
```

## 测试策略
- 现有 `ffmpeg_test.cpp` 应全部通过
- 集成测试验证视频处理流程
- 内存泄漏检测（可选）

## 验收标准
- [ ] 消除所有手动 `cleanup()` 调用
- [ ] 所有 FFmpeg 资源使用 RAII 管理
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] 无资源泄漏（可通过 Valgrind 验证）
