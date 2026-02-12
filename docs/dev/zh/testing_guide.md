# 测试指南 (Testing Guide)

FaceFusionCpp 采用全面的测试策略来确保可靠性和性能。本指南解释了测试框架以及如何编写新测试。

## 1. 测试分类 (Test Categories)

我们使用 Google Test (GTest) 并将测试分为三个级别：

### 1.1 单元测试 (Unit Tests - `tests/unit/`)
*   **目的**: 验证独立模块（函数、类）的功能。
*   **速度**: 非常快 (< 1秒)。
*   **依赖**: 最小化。使用 Mock 模拟外部依赖（文件系统、AI 模型）。
*   **示例**: `test_config_parser.cpp`, `test_image_utils.cpp`。

### 1.2 集成测试 (Integration Tests - `tests/integration/`)
*   **目的**: 验证多个组件之间的交互（例如：流水线运行器 + 处理器）。
*   **速度**: 中等（几秒到几分钟）。
*   **依赖**: 真实的文件系统，真实的 AI 模型（或虚拟模型）。
*   **示例**: `test_pipeline_runner.cpp`（在样本图像上执行完整流水线）。

### 1.3 端到端测试 (E2E Tests - `tests/e2e/`)
*   **目的**: 从用户角度验证应用程序行为（CLI 输入 -> 输出文件）。
*   **速度**: 慢。
*   **依赖**: 完整的系统环境。
*   **示例**: `run_e2e.py` 脚本针对测试场景执行二进制文件。

## 2. 运行测试 (Running Tests)

我们使用 `build.py` 来运行测试。

### 2.1 运行所有测试
```bash
python build.py --action test
```

### 2.2 运行特定类别
您可以通过标签过滤测试：
```bash
# 仅运行单元测试
python build.py --action test --test-label unit

# 仅运行集成测试
python build.py --action test --test-label integration
```

### 2.3 运行特定测试用例 (GTest 过滤器)
要运行特定的测试套件或用例：
```bash
# 语法: --gtest_filter=SuiteName.TestName
./build/bin/FaceFusionCpp_Test --gtest_filter=ConfigTest.*
```

## 3. 编写测试 (Writing Tests)

### 3.1 基本结构
在 `tests/unit/your_module/` 中创建一个新的 `.cpp` 文件。

```cpp
#include <gtest/gtest.h>
import your.module;

TEST(YourModuleTest, ShouldDoSomething) {
    // Arrange (准备)
    int input = 5;
    
    // Act (执行)
    int result = your::module::process(input);
    
    // Assert (断言)
    EXPECT_EQ(result, 10);
}
```

### 3.2 使用测试夹具 (Test Fixtures)
用于需要 设置/拆解 的测试（例如，创建临时文件）。

```cpp
class FileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录
    }

    void TearDown() override {
        // 清理临时目录
    }
};

TEST_F(FileSystemTest, FileExists) {
    // ...
}
```

### 3.3 路径辅助 (Path Helpers)
使用 `tests/common/test_paths.h` 安全地管理文件路径。

```cpp
#include "common/test_paths.h"

std::string model_path = tests::common::GetTestModelPath("inswapper_128.onnx");
std::string output_path = tests::common::GetTestOutputPath("result.jpg");
```

## 4. 最佳实践 (Best Practices)
1.  **隔离性**: 测试不应相互依赖。
2.  **清理**: 始终在 `TearDown` 中清理创建的文件。
3.  **Mocking**: 在单元测试中使用 GMock 模拟昂贵的操作（如 AI 推理）。
4.  **命名**: 使用 `SuiteName_TestName` 格式。
