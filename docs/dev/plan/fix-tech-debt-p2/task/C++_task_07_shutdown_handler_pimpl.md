# 子任务 7: shutdown_handler.ixx PIMPL 封装

## 基本信息
- **所属计划**: fix-tech-debt-p2
- **优先级**: P2
- **修改文件**: `src/services/pipeline/shutdown_handler.ixx`, `src/services/pipeline/shutdown_handler.cpp`
- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 5e03989

## 目标
将平台相关实现和私有成员隐藏到 `.cpp`，实现 PIMPL（Pointer to Implementation）模式。

## 研究发现

### 当前问题
1. `.ixx` 文件中包含 `#include <Windows.h>` 和 `#ifdef _WIN32` 平台相关代码
2. 私有成员直接暴露在接口中，破坏封装性
3. 修改私有成员会导致所有依赖文件重新编译

### PIMPL 模式优势
1. **编译防火墙**：修改实现不影响头文件，减少重编译
2. **平台隔离**：平台相关代码完全隐藏在 `.cpp`
3. **ABI 稳定性**：接口不变，二进制兼容性更好

## 具体改动

### 1. 在 `.ixx` 中添加前向声明
```cpp
// 前向声明
struct ShutdownHandlerImpl;
```

### 2. 替换私有成员为 PIMPL 指针
```cpp
class ShutdownHandler {
public:
    // ... 公有接口不变 ...
private:
    std::unique_ptr<ShutdownHandlerImpl> pImpl;
};
```

### 3. 在 `.cpp` 中实现 PIMPL
```cpp
struct ShutdownHandlerImpl {
    // 所有原私有成员移到这里
    std::atomic<ShutdownState> state{ShutdownState::Running};
    std::function<void()> shutdown_callback;
    std::function<void()> timeout_callback;
    std::chrono::milliseconds timeout;
    std::mutex mutex;
    std::condition_variable cv;
    std::thread timeout_thread;
    
    // 平台相关成员
#ifdef _WIN32
    // Windows 特定成员
#else
    // POSIX 特定成员
#endif
};
```

### 4. 修改构造函数和析构函数
```cpp
ShutdownHandler::ShutdownHandler(/* params */)
    : pImpl(std::make_unique<ShutdownHandlerImpl>()) {
    // 初始化 pImpl 成员
}

ShutdownHandler::~ShutdownHandler() = default;
```

## 测试策略
- 现有 `shutdown_handler_test.cpp` 应全部通过
- 编译验证跨平台兼容性

## 验收标准
- [ ] `.ixx` 中无 `#include <Windows.h>` 或 `#ifdef _WIN32`
- [ ] 所有私有成员移入 PIMPL
- [ ] 单元测试通过
- [ ] 编译通过（Linux 和 Windows）
