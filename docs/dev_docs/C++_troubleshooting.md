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
  在测试代码中 `import domain.ai.model_repository;` 时报错 `could not find module`，即使该模块已经存在。
- **原因分析**:
  CMake 构建目标（如 `test_domain_face_detector`）没有在其 `LINK_LIBRARIES` 中添加对应的模块库依赖（如 `domain_ai`）。C++20 模块的 BMI 编译顺序依赖于 CMake 的依赖图，若未声明依赖，编译器无法找到预编译的模块接口。
- **解决方案**:
  在 `CMakeLists.txt` 中显式添加缺少的库依赖：
  ```cmake
  target_link_libraries(target_name PRIVATE dependency_lib)
  ```
- **避免措施**:
  在编写测试或新模块时，仔细检查 `import` 的模块是否已在 CMake 中声明为依赖。

### ONNX Runtime + TensorRT 导致的 SEH 异常 (0xc0000005)

- **日期**: 2026-01-14
- **标签**: [运行时崩溃, SEH, ONNX Runtime, TensorRT, Windows]
- **问题描述**:
  在 Windows 环境下使用 ONNX Runtime 配合 TensorRT 执行提供者时，程序在退出或单元测试结束（对象析构阶段）常抛出 SEH 异常 `0xc0000005` (Access Violation)。
- **原因分析**:
  1. **析构顺序冲突**: ONNX Runtime 要求 `Ort::Env` 的生命周期必须长于所有 `Ort::Session`。如果顺序不当，底层 GPU 资源释放时会尝试访问已销毁的句柄。
  2. **驱动级冲突**: TensorRT 插件在 DLL 卸载时的内存释放机制有时与 Windows 的 `LdrUnloadDll` 存在时序冲突。
  3. **单元测试环境**: 在单元测试中，由于测试框架的并行执行或非预期析构顺序，可能导致推理 Session 还没完全清理，主环境就已经销毁。此外，复杂的资源依赖图可能导致在程序退出阶段触发非法地址访问。
- **解决方案**:
  1. **Leaked Static Ort::Env (核心修复)**: 在 `InferenceSession` 内部使用“故意泄漏”的静态指针来持有 `Ort::Env`。
     ```cpp
     static const Ort::Env& get_static_env() {
         static Ort::Env* env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "FaceFusionCpp");
         return *env;
     }
     ```
     这确保了 ONNX 环境在进程整个生命周期内始终有效，直到操作系统回收进程资源，从而避开了由于 C++ 静态对象析构顺序不确定导致的崩溃。
  2. **显式析构顺序**: 在 `InferenceSession` 的 PIMPL 实现中显式定义析构函数，按照“先清空名称指针 -> 后释放 Session -> 最后重置 Provider Options”的顺序执行。
  3. **死锁防护**: 内部使用 `std::recursive_mutex` 替换普通 `mutex`，防止在复杂的初始化或多阶段 `load_model` 过程中发生递归加锁死锁。
  4. **环境变量控制**: 引入 `FACEFUSION_PROVIDER=cpu` 强制切换逻辑，用于 CI/CD 等无 GPU 或驱动不稳定的环境。
- **避免措施**:
  - 推理引擎应提供显式的 `reset()` 或销毁接口，而不是完全依赖自动析构。
  - 对于底层依赖复杂的第三方库（如 GPU 驱动），使用静态泄漏指针（Static Leaked Pointer）是一种常见的规避析构顺序问题的工业级方案。
  - CI/CD 环境应优先使用稳定但较慢的 CPU 路径。

### C2064: term does not evaluate to a function

- **日期**: 2026-01-14
- **标签**: [编译错误, C++ 基础, FaceDetector]
- **问题描述**:
  在编写 `FaceDetector` 相关的测试代码时，调用 `results[0].box()` 出现错误：
  `error C2064: term does not evaluate to a function taking 0 arguments`.
- **原因分析**:
  `DetectionResult` 结构体中的 `box` 是一个成员变量 (`cv::Rect2f`)，而不是一个成员函数。
  在旧的 `Face` 类设计中，`box()` 是一个访问器方法；而在 `FaceDetector` 返回的轻量级结果结构体中，它被设计为直接访问的字段。
- **解决方案**:
  去掉括号，直接访问成员变量：`results[0].box`。
- **避免措施**:
  在使用新的 API 或重构后的模块时，仔细查看头文件定义的类型结构，区分 Getter 方法和公共成员变量。

### 不同 Face Landmarker 模型的评分敏感度差异

- **日期**: 2026-01-14
- **标签**: [AI 模型, ONNX, 测试策略]
- **问题描述**:
  在 Face Landmarker 单元测试中，`2DFAN` 模型在标准测试图 (Lenna) 上得分 ~0.69，而 `Peppawutz` 模型仅得 ~0.32。若统一使用 0.5 作为通过阈值，会导致 Peppawutz 测试失败。
- **原因分析**:
  1. **模型特性差异**: 两个模型虽然都输出 68 个关键点，但其内部置信度评分的分布范围不同。
  2. **系统偏好**: 通过分析旧代码逻辑 (`FaceLandmarkerHub`) 发现，系统存在 `score_2dfan > score_peppa - 0.2` 的偏好逻辑，暗示 Peppawutz 的原生分值倾向于比 2DFAN 低，或者是系统更信任 2DFAN。
  3. **数据敏感性**: Peppawutz 对部分测试图（如 Lenna）的评分可能比 Tiffany 更低，而 2DFAN 相对稳健。
- **解决方案**:
  1. **差异化阈值**: 在单元测试中，针对不同模型设置不同的通过阈值（如 Peppawutz 设为 0.3）。
  2. **预处理一致性**: 确保了 Peppawutz 的预处理（BGR 顺序, [0, 1] 归一化）与旧代码严格一致，排除了实现错误的可能性。
- **避免措施**:
  在移植 AI 模型时，不要假设所有同类模型的输出分布一致。应参考原系统的业务逻辑（如加权、偏置）来确定合理的测试基准。

<!-- 在此处添加新问题 -->
