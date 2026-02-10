# FaceFusionCpp 测试框架

本目录包含 FaceFusionCpp 项目的所有单元测试、集成测试和性能测试。

## 目录结构

测试目录重新组织为以下结构：

```
tests/
├── CMakeLists.txt              # 测试构建配置
├── README.md                   # 本文件
├── unit/                       # 真正的单元测试 (纯 Mock/无外部依赖)
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
├── e2e/                        # 端到端测试 (预留)
```

## 测试类型标准

### 1. 单元测试 (Unit)
- **位置**: `tests/unit/`
- **依赖**: 无外部文件、无网络、无真实模型
- **特点**: 毫秒级执行，大量使用 Mock/Stub
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
2. 如果测试需要加载真实模型、读取图片/视频 -> `tests/integration`
3. 如果测试目的是测量性能 -> `tests/benchmark`

使用 `add_facefusion_test` 宏添加测试时，系统会自动应用相应的标签。
