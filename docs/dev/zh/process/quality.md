# C++ 质量标准与开发检查单 (Quality Standards & Checklist)

## 1. 核心原则

- **C++20 强制**: 必须使用 C++20 特性（如 Modules, Concepts, std::span, std::format）。
- **模块化隔离**: 严格遵循 `.ixx` (接口) 与 `.cpp` (实现) 分离的原则。
- **RAII 极致**: 所有资源（显存、内存、句柄）必须由智能指针或自定义 RAII 包装器管理。

## 2. 编码规范

### 2.1 命名约定
- **类/结构体**: `PascalCase`
- **函数/方法**: `snake_case`
- **变量**: `snake_case`
- **成员变量**: `m_snake_case`
- **常量**: `kPascalCase`

### 2.2 防御性编程
- 优先使用 `Early Return` 减少嵌套层级。
- 严禁静默吞掉异常，所有异常必须在合适的层级被捕获并转化为错误码（Exxx）或记录日志。
- 对第三方库（FFmpeg, ONNX Runtime）的调用必须有完善的错误检查。

## 3. 开发检查单 (Pre-Commit Checklist)

在提交 PR 之前，请确保：

- [ ] **编译无误**: 在 Linux 和 Windows 上通过 `python build.py --action build`。
- [ ] **单元测试**: 运行 `python build.py --action test --test-label unit` 且全部通过。
- [ ] **格式化**: 运行过 `scripts/format_code.py`。
- [ ] **模块化**: 新增类是否使用了 C++20 模块？是否隐藏了实现细节？
- [ ] **路径解析**: 是否使用了 `platform.fs` 提供的相对路径解析，而非硬编码绝对路径？
- [ ] **日志**: 关键逻辑节点是否有 `INFO` 或 `DEBUG` 级别的耗时日志？
