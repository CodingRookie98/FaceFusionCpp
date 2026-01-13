# FaceFusionCpp 测试框架

本目录包含 FaceFusionCpp 项目的所有单元测试和集成测试。

## 目录结构

测试目录结构与源代码目录 (`src/`) 结构保持镜像一致，主要分为单元测试 (`unit`) 和集成测试 (`integration`)。

```
tests/
├── CMakeLists.txt              # 测试构建配置
├── README.md                   # 本文件
├── unit/                       # 单元测试（对应 src 目录结构）
│   ├── CMakeLists.txt
│   ├── app/                    # 对应 src/app
│   ├── domain/                 # 对应 src/domain
│   │   ├── face/
│   │   │   ├── detector/
│   │   │   ├── face_helper_test.cpp
│   │   │   └── ...
│   │   └── ai/
│   ├── foundation/             # 对应 src/foundation
│   ├── platform/               # 对应 src/platform
│   └── services/               # 对应 src/services
└── integration/                # 集成测试（如需）
```


## 快速开始

### 1. 启用测试构建

默认情况下，测试是启用的。如果需要禁用：

```bash
cmake -DBUILD_TESTING=OFF ..
```

### 2. 构建测试

```bash
# 在构建目录中
cmake --build . --target all
```

### 3. 运行所有测试

```bash
# 使用 CTest
ctest --output-on-failure

# 或者直接运行测试可执行文件
./tests/example_test
```

### 4. 运行特定测试

```bash
# 运行特定测试用例
./tests/example_test --gtest_filter=ExampleTest.BasicAssertions

# 运行特定测试套件
./tests/example_test --gtest_filter=ExampleTest.*

# 使用通配符
./tests/example_test --gtest_filter=*Counter*
```

## 编写新测试

### 1. 创建测试文件

在 `tests/` 目录下创建新的 `.cpp` 文件，例如 `core_test.cpp`。

### 2. 包含必要的头文件

```cpp
#include <gtest/gtest.h>
#include "your_module_header.h"
```

### 3. 编写测试用例

```cpp
TEST(YourTestSuite, YourTestCase) {
    // 准备测试数据
    int expected = 42;

    // 执行被测试的代码
    int actual = your_function();

    // 验证结果
    EXPECT_EQ(actual, expected);
}
```

### 4. 在 CMakeLists.txt 中注册测试

编辑 `tests/CMakeLists.txt`，添加新的测试：

```cmake
add_facefusion_test(
    core_test
    SOURCES
        core_test.cpp
    LINK_LIBRARIES
        facefusion_core  # 如果需要链接项目库
)
```

## GoogleTest 基础

### 断言类型

- **EXPECT_***: 失败后继续执行后续测试
- **ASSERT_***: 失败后立即终止当前测试用例

### 常用断言

```cpp
// 布尔值
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// 数值比较
EXPECT_EQ(val1, val2);  // 相等
EXPECT_NE(val1, val2);  // 不相等
EXPECT_LT(val1, val2);  // 小于
EXPECT_LE(val1, val2);  // 小于等于
EXPECT_GT(val1, val2);  // 大于
EXPECT_GE(val1, val2);  // 大于等于

// 字符串比较
EXPECT_STREQ(str1, str2);      // C 字符串相等
EXPECT_STRNE(str1, str2);      // C 字符串不相等
EXPECT_EQ(str1, str2);         // std::string 相等

// 浮点数比较
EXPECT_FLOAT_EQ(float1, float2);
EXPECT_DOUBLE_EQ(double1, double2);
EXPECT_NEAR(val1, val2, abs_error);

// 异常测试
EXPECT_THROW(statement, exception_type);
EXPECT_ANY_THROW(statement);
EXPECT_NO_THROW(statement);
```

### 测试夹具 (Test Fixtures)

```cpp
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 在每个测试前执行
    }

    void TearDown() override {
        // 在每个测试后执行
    }

    // 共享数据成员
    MyClass* test_object;
};

TEST_F(MyTestFixture, Test1) {
    // 使用 test_object
}
```

### 参数化测试

```cpp
class MyParameterizedTest : public ::testing::TestWithParam<int> {
};

TEST_P(MyParameterizedTest, DoesSomething) {
    int param = GetParam();
    EXPECT_GT(param, 0);
}

INSTANTIATE_TEST_SUITE_P(
    PositiveNumbers,
    MyParameterizedTest,
    ::testing::Values(1, 2, 3, 4, 5)
);
```

## 高级功能

### 测试发现

项目使用 `gtest_discover_tests()` 自动发现测试，这意味着：
- 不需要在 CMake 中手动列出每个测试用例
- 添加新测试后无需重新运行 CMake
- 支持参数化测试

### 测试标签

所有测试都标记为 `facefusion`，可以按标签运行：

```bash
ctest -L facefusion
```

### 超时设置

每个测试的默认超时时间为 300 秒。可以在 `tests/CMakeLists.txt` 中修改：

```cmake
set_tests_properties(${TEST_NAME} PROPERTIES
    TIMEOUT 600  # 设置为 600 秒
)
```

## 最佳实践

1. **命名规范**
   - 测试套件使用描述性名称：`CoreTest`, `FaceDetectorTest`
   - 测试用例描述具体行为：`DetectFaces_ReturnsCorrectCount`

2. **测试独立性**
   - 每个测试应该独立运行
   - 不依赖其他测试的执行顺序
   - 在 `SetUp()` 和 `TearDown()` 中清理资源

3. **测试覆盖率**
   - 测试正常路径
   - 测试边界条件
   - 测试错误情况

4. **性能考虑**
   - 对于耗时测试，考虑使用 `DISABLED_` 前缀
   - 使用 `--gtest_filter` 跳过慢速测试

5. **文档**
   - 为复杂的测试添加注释
   - 说明测试的目的和预期行为

## 调试测试

### 启用详细输出

```bash
./tests/example_test --gtest_print_time=1
```

### 重复运行测试

```bash
# 重复运行 100 次（用于发现间歇性失败）
./tests/example_test --gtest_repeat=100
```

### 只运行失败的测试

```bash
./tests/example_test --gtest_filter=*:*
```

## 故障排除

### 问题：找不到 GTest

确保：
1. `BUILD_TESTING` 选项已启用
2. CMake 版本 >= 3.14
3. 网络连接正常（需要下载 GoogleTest）

### 问题：测试编译失败

检查：
1. 所有头文件路径正确
2. 链接了必要的库
3. C++ 标准设置为 C++20

### 问题：测试运行时崩溃

使用调试器：

```bash
gdb ./tests/example_test
# 或者在 CMake 中启用调试符号
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## 参考资料

- [GoogleTest Primer](https://google.github.io/googletest/primer.html)
- [GoogleTest Advanced Guide](https://google.github.io/googletest/advanced.html)
- [GoogleTest FAQ](https://google.github.io/googletest/faq.html)
