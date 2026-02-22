# C++ Quality Standards & Development Checklist

## 1. Core Principles

- **C++20 Mandatory**: Must use C++20 features (e.g., Modules, Concepts, std::span, std::format).
- **Modular Isolation**: Strictly follow the principle of separating `.ixx` (interface) and `.cpp` (implementation).
- **Extreme RAII**: All resources (VRAM, RAM, handles) must be managed by smart pointers or custom RAII wrappers.

## 2. Coding Standards

### 2.1 Naming Conventions
- **Classes/Structs**: `PascalCase`
- **Functions/Methods**: `snake_case`
- **Variables**: `snake_case`
- **Member Variables**: `m_snake_case`
- **Constants**: `kPascalCase`

### 2.2 Defensive Programming
- Prefer `Early Return` to reduce nesting.
- Never silently swallow exceptions; all exceptions must be caught at the appropriate level and translated into error codes (Exxx) or logged.
- Calls to third-party libraries (FFmpeg, ONNX Runtime) must have thorough error checking.

## 3. Development Checklist (Pre-Commit)

Before submitting a PR, ensure:

- [ ] **Compilation**: Passes on both Linux and Windows via `python build.py --action build`.
- [ ] **Unit Tests**: Run `python build.py --action test --test-label unit` and pass all.
- [ ] **Formatting**: Run `scripts/format_code.py`.
- [ ] **Modularity**: Are new classes using C++20 modules? Is implementation hidden?
- [ ] **Path Resolution**: Use `platform.fs` for relative path resolution instead of hardcoding absolute paths.
- [ ] **Logging**: Do critical logic nodes have `INFO` or `DEBUG` level timing logs?
