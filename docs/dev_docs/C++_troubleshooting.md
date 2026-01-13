# C++ 开发疑难杂症与解决方案记录

本文档用于记录在 C++ 项目开发过程中遇到的疑难问题、踩坑经验以及需要避免的操作。

## 记录模板

### [问题标题]

- **日期**: YYYY-MM-DD
- **标签**: [例如: 编译错误, 运行时崩溃, 内存泄漏, MSVC, C++20]
- **问题描述**:
  详细描述遇到的问题，包括错误信息、复现步骤等。
- **原因分析**:
  分析导致问题的原因。
- **解决方案**:
  提供具体的解决办法或代码片段。
- **避免措施**:
  后续开发中如何避免类似问题。

---

## 问题列表

### LNK2019: 模块导出函数的实现被声明为 static

- **日期**: 2026-01-14
- **标签**: [编译错误, C++20 Modules, LNK2019]
- **问题描述**:
  在 `.ixx` 接口文件中使用 `export` 导出了函数（如 `to_json`），但在 `.cpp` 实现文件中将该函数定义为 `static`。导致链接时报错 `unresolved external symbol`。
- **原因分析**:
  `static` 关键字在全局/命名空间作用域中意味着内部链接（Internal Linkage），使得函数仅在当前翻译单元可见。即使在接口文件中声明了导出，实现文件的 `static` 也会导致符号对外部不可见。
- **解决方案**:
  移除 `.cpp` 实现文件中的 `static` 关键字。
- **避免措施**:
  实现模块导出函数时，确保不要在实现文件中错误地添加 `static` 修饰符。

### C2230: could not find module 'xxx'

- **日期**: 2026-01-14
- **标签**: [编译错误, C++20 Modules, CMake]
- **问题描述**:
  在测试代码中 `import domain.ai.model_manager;` 时报错 `could not find module`，即使该模块已经存在。
- **原因分析**:
  CMake 构建目标（如 `test_domain_face_detector`）没有在其 `LINK_LIBRARIES` 中添加对应的模块库依赖（如 `domain_ai`）。C++20 模块的 BMI 编译顺序依赖于 CMake 的依赖图，若未声明依赖，编译器无法找到预编译的模块接口。
- **解决方案**:
  在 `CMakeLists.txt` 中显式添加缺少的库依赖：
  ```cmake
  target_link_libraries(target_name PRIVATE dependency_lib)
  ```
- **避免措施**:
  在编写测试或新模块时，仔细检查 `import` 的模块是否已在 CMake 中声明为依赖。

<!-- 在此处添加新问题 -->
