# Testing Guide

FaceFusionCpp uses a comprehensive testing strategy to ensure reliability and performance. This guide explains the testing framework and how to write new tests.

## 1. Test Categories

We use Google Test (GTest) and categorize tests into three levels:

### 1.1 Unit Tests (`tests/unit/`)
*   **Purpose**: Verify individual modules (functions, classes) in isolation.
*   **Speed**: Very Fast (< 1s).
*   **Dependencies**: Minimal. Mocks are used for external dependencies (filesystem, AI models).
*   **Example**: `test_config_parser.cpp`, `test_image_utils.cpp`.

### 1.2 Integration Tests (`tests/integration/`)
*   **Purpose**: Verify interaction between multiple components (e.g., Pipeline Runner + Processor).
*   **Speed**: Moderate (seconds to minutes).
*   **Dependencies**: Real file system, real AI models (or dummy models).
*   **Example**: `test_pipeline_runner.cpp` (executes a full pipeline on a sample image).

### 1.3 End-to-End (E2E) Tests (`tests/e2e/`)
*   **Purpose**: Verify the application behavior from a user's perspective (CLI inputs -> Output files).
*   **Speed**: Slow.
*   **Dependencies**: Full system environment.
*   **Example**: `run_e2e.py` script executing the binary against test scenarios.

## 2. Running Tests

We use `build.py` to run tests.

### 2.1 Run All Tests
```bash
python build.py --action test
```

### 2.2 Run Specific Category
You can filter tests by label:
```bash
# Run unit tests only
python build.py --action test --test-label unit

# Run integration tests only
python build.py --action test --test-label integration
```

### 2.3 Run Specific Test Case (GTest Filter)
To run a specific test suite or case:
```bash
# Syntax: --gtest_filter=SuiteName.TestName
./build/bin/FaceFusionCpp_Test --gtest_filter=ConfigTest.*
```

## 3. Writing Tests

### 3.1 Basic Structure
Create a new `.cpp` file in `tests/unit/your_module/`.

```cpp
#include <gtest/gtest.h>
import your.module;

TEST(YourModuleTest, ShouldDoSomething) {
    // Arrange
    int input = 5;
    
    // Act
    int result = your::module::process(input);
    
    // Assert
    EXPECT_EQ(result, 10);
}
```

### 3.2 Using Test Fixtures
For tests requiring setup/teardown (e.g., creating temp files).

```cpp
class FileSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temp dir
    }

    void TearDown() override {
        // Cleanup temp dir
    }
};

TEST_F(FileSystemTest, FileExists) {
    // ...
}
```

### 3.3 Path Helpers
Use `tests/common/test_paths.h` to manage file paths safely.

```cpp
#include "common/test_paths.h"

std::string model_path = tests::common::GetTestModelPath("inswapper_128.onnx");
std::string output_path = tests::common::GetTestOutputPath("result.jpg");
```

## 4. Best Practices
1.  **Isolation**: Tests should not depend on each other.
2.  **Clean Up**: Always clean up created files in `TearDown`.
3.  **Mocking**: Use GMock to mock expensive operations (like AI inference) in Unit Tests.
4.  **Naming**: Use `SuiteName_TestName` format.
