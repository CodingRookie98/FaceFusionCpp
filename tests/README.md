# FaceFusionCpp 测试框架

本目录包含 FaceFusionCpp 项目的所有单元测试、集成测试和性能测试。

## 目录结构

测试目录重新组织为以下结构：

```
tests/
├── CMakeLists.txt              # 测试构建配置
├── README.md                   # 本文件
├── common/                     # 通用测试基础设施 (Fixtures, Matchers, Generators)
├── helpers/                    # 测试辅助工具 (Env, Monitor, Constants)
├── mocks/                      # 集中管理的 Mock 对象库
├── unit/                       # 单元测试 (纯 Mock/无外部依赖)
│   ├── app/
│   ├── domain/
│   ├── foundation/
│   └── ...
├── integration/                # 集成测试 (依赖真实模型/文件)
│   ├── app/
│   ├── domain/
│   ├── foundation/
│   └── ...
├── benchmark/                  # 性能基准测试
│   └── app/
└── data/                       # 测试用数据 (Images, Models)
```

## 测试类型标准

### 1. 单元测试 (Unit)
- **位置**: `tests/unit/`
- **依赖**: 无外部文件、无网络、无真实模型
- **特点**: 毫秒级执行，大量使用 `tests/mocks` 中的对象
- **运行**: `ctest -L unit`

### 2. 集成测试 (Integration)
- **位置**: `tests/integration/`
- **依赖**: 使用真实模型、图片、视频文件
- **特点**: 秒级执行，验证组件交互
- **运行**: `ctest -L integration`

### 3. 性能测试 (Benchmark)
- **位置**: `tests/benchmark/`
- **目的**: 测量 FPS、延迟、内存等性能指标
- **构建**: 默认不构建，需 `cmake -DBUILD_BENCHMARK_TESTS=ON ..`
- **运行**: `ctest -L benchmark`

### 4. 测试输出 (Test Output)
所有测试的输出文件现已统一存放，不再散落在项目根目录。
- **路径规则**: `build/{preset}/bin/output/test/{category}/`
- **示例**:
  - 单元测试: `.../bin/output/test/checkpoint_manager/`
  - E2E测试: `.../bin/output/test/e2e/e2e_image_single/`

## 基础设施与工具

### Common (`tests/common/`)
包含所有测试共享的基础组件：
- **Fixtures**: `BaseTestFixture`, `UnitTestFixture` (自动 Mock 清理), `IntegrationTestFixture` (自动资源路径/全局环境)
- **Matchers**: `OpenCVMatchers` (如 `MatEq`)
- **Generators**: `create_valid_face()`, `create_black_image()` 等数据生成器

### Helpers (`tests/helpers/`)
包含测试辅助逻辑：
- **Environment**: `GlobalTestEnvironment`
- **Foundation**: `MemoryMonitor`, `NvmlMonitor` (GPU), `PerformanceValidator`

### Mocks (`tests/mocks/`)
集中管理的 Mock 对象，避免重复定义：
- `MockInferenceSession`
- `MockFaceDetector`, `MockFaceEnhancer`, `MockModelRepository`

## 运行测试

### 使用 build.py (推荐)

项目提供了构建脚本 `build.py` 来简化测试运行：

```bash
# 运行所有测试
python build.py --action test

# 运行单元测试
python build.py --action test --test-label unit

# 运行集成测试
python build.py --action test --test-label integration
```

### 手动运行命令

```bash
# 运行所有常规测试 (Unit + Integration)
ctest --output-on-failure

# 只运行单元测试
ctest -L unit --output-on-failure

# 只运行集成测试
ctest -L integration --output-on-failure

# 运行性能测试
ctest -L benchmark
```

## 编写新测试

请遵循以下原则决定测试存放位置：
1. 如果测试仅涉及逻辑验证，不需要加载模型文件或图片 -> `tests/unit`
   - 继承 `UnitTestFixture`
   - 使用 `tests/mocks`
2. 如果测试需要加载真实模型、读取图片/视频 -> `tests/integration`
   - 继承 `IntegrationTestFixture`
   - 使用 `tests/helpers`
3. 如果测试目的是测量性能 -> `tests/benchmark`

使用 `add_facefusion_test` 宏添加测试时，系统会自动应用相应的标签。

## 最佳实践与建议 (Next Steps)

为保持测试框架的健康和可维护性，建议遵循以下最佳实践：

### 1. 使用生成器替代硬编码数据
在编写新的测试或重构旧测试时，请优先使用 `tests/common/generators` 模块。
*   **推荐**:
    ```cpp
    import tests.common.generators.face_generator;
    auto face = tests::common::generators::create_valid_face();
    ```
*   **避免**:
    ```cpp
    // 避免手动构造复杂对象
    domain::face::Face face;
    face.set_embedding(std::vector<float>(512, 0.0f));
    ```

### 2. 集成测试继承 IntegrationTestFixture
编写需要 GPU 环境或资源加载的集成测试时，**必须**继承 `tests::common::fixtures::IntegrationTestFixture`。
*   该 Fixture 会自动处理 `GlobalTestEnvironment` 的链接和生命周期，防止 TensorRT/CUDA 上下文清理时的崩溃问题。
*   它还提供了便捷的 `GetAssetsPath()` 和 `GetTestDataPath()` 方法。

### 3. 优先使用 C++20 Modules
新添加的测试辅助代码应编写为 C++20 Modules (`.ixx`)，而不是传统的头文件 (`.hpp`)，以减少编译依赖和命名空间污染。
