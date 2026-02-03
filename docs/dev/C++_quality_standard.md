# C++代码质量与评估标准指南

本文档定义了 FaceFusionCpp 项目的 C++ 代码质量标准、评估工具使用指南以及详细的评分规则。

## 1. Doxygen 注释标准

> **说明**：所有 Public Module Interface (.ixx) 必须遵循以下 Doxygen 注释规范。

### 1.1 文件头模板
每个 `.ixx` 文件开头必须包含：
```cpp
/**
 * @file {filename}
 * @brief {Short description}
 * @author CodingRookie
 * @date {YYYY-MM-DD}
 * @note {Optional detailed notes}
 */
```

### 1.2 类与接口模板
所有导出的类和公共方法必须包含：
```cpp
/**
 * @brief {Brief description}
 * @details {Detailed description if needed}
 * @param {name} {description}
 * @return {description}
 */
```

### 1.3 示例 (InferenceSession)
```cpp
/**
 * @file inference_session.ixx
 * @brief ONNX Runtime inference session wrapper module
 * @author CodingRookie
 * @date 2024-07-12
 */

export module foundation.ai.inference_session;

namespace foundation::ai::inference_session {

    /**
     * @brief ONNX Runtime inference session wrapper class
     * @details Provides high-level interface for loading ONNX models...
     */
    export class InferenceSession {
    public:
        /**
         * @brief Load an ONNX model with specified options
         * @param model_path Path to the ONNX model file
         * @param options Session options
         */
        virtual void load_model(const std::string& model_path, const Options& options);
    };
}
```

## 2. 评估工具使用指南

### 2.1 静态分析工具
> **说明**：介绍用于代码质量评估的静态分析工具

#### 2.1.1 编译器诊断工具

**MSVC 编译器诊断**:
```bash
# 启用所有警告
cl /W4 /Wall /permissive- /std:c++20 /EHsc

# 启用额外警告
cl /W4 /Wall /permissive- /std:c++20 /EHsc /external:W0

# 输出诊断信息
cl /W4 /permissive- /std:c++20 /EHsc /diagnostics:caret
```

**GCC/Clang 编译器诊断**:
```bash
# 启用所有警告
g++ -Wall -Wextra -Wpedantic -std=c++20

# 启用额外警告
g++ -Wall -Wextra -Wpedantic -std=c++20 -Wconversion -Wshadow

# 输出诊断信息
g++ -Wall -Wextra -Wpedantic -std=c++20 -fdiagnostics-color=always
```

#### 2.1.2 静态分析工具

**Clang-Tidy**:
```bash
# 基本检查
clang-tidy src/**/*.cpp -- -std=c++20 -I./include

# 启用所有检查
clang-tidy src/**/*.cpp --checks='*' -- -std=c++20 -I./include

# 使用 .clang-tidy 配置文件
clang-tidy src/**/*.cpp
```

**Cppcheck**:
```bash
# 基本检查
cppcheck --enable=all --std=c++20 --inconclusive src/

# 详细输出
cppcheck --enable=all --std=c++20 --inconclusive --verbose src/

# 输出到文件
cppcheck --enable=all --std=c++20 --inconclusive --xml src/ 2> report.xml
```

**Include-What-You-Use**:
```bash
# 分析头文件依赖
include-what-you-use src/**/*.cpp -std=c++20 -I./include

# 输出建议
include-what-you-use -Xiwyu --verbose=1 src/**/*.cpp -std=c++20 -I./include
```

#### 2.1.3 模块特定工具

**模块依赖分析**:
```bash
# 生成 Graphviz 依赖图
cmake --graphviz=module_deps.dot .
dot -Tpng module_deps.dot -o module_deps.png
```

**BMI (.ifc) 分析**:
```bash
# 检查 BMI 文件大小 (PowerShell)
Get-ChildItem -Path build -Filter *.ifc -Recurse | Select-Object Name, @{Name="MB";Expression={$_.Length / 1MB}} | Sort-Object Length -Descending

# 检查是否包含不必要的符号 (dumpbin /headers)
dumpbin /headers build/path/to/module.ifc
```

### 2.2 动态分析工具
> **说明**：介绍用于运行时分析的动态分析工具

#### 2.2.1 内存分析工具

**AddressSanitizer (ASan)**:
```bash
# 编译时启用 ASan
g++ -fsanitize=address -fno-omit-frame-pointer -g -std=c++20 src/*.cpp

# MSVC 启用 ASan
cl /fsanitize=address /std:c++20 src/*.cpp
```

**Valgrind Memcheck**:
```bash
# 内存泄漏检测
valgrind --leak-check=full --show-leak-kinds=all ./your_program

# 详细报告
valgrind --leak-check=full --show-leak-kinds=all --verbose ./your_program
```

#### 2.2.2 线程分析工具

**ThreadSanitizer (TSan)**:
```bash
# 编译时启用 TSan
g++ -fsanitize=thread -g -std=c++20 src/*.cpp

# 运行时检测数据竞争
./a.out
```

**Helgrind**:
```bash
# 线程错误检测
valgrind --tool=helgrind ./your_program

# 详细报告
valgrind --tool=helgrind --verbose ./your_program
```

#### 2.2.3 性能分析工具

**perf**:
```bash
# CPU 性能分析
perf record -g ./your_program

# 查看报告
perf report

# 火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

**Visual Studio Profiler**:
```cpp
// 使用性能分析器
// 1. 在 Visual Studio 中打开项目
// 2. 选择 "调试" -> "性能分析器"
// 3. 选择分析类型（CPU 使用率、内存使用等）
// 4. 运行分析
```

### 2.3 测试工具
> **说明**：介绍用于测试评估的工具

#### 2.3.1 单元测试框架

**Google Test**:
```cpp
// 基本测试
#include <gtest/gtest.h>

TEST(TestCaseName, TestName) {
    EXPECT_EQ(1, 1);
    ASSERT_TRUE(true);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

**Catch2**:
```cpp
// 基本测试
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Test case name") {
    REQUIRE(1 == 1);
    CHECK(true);
}
```

#### 2.3.2 代码覆盖率工具

**gcov/lcov**:
```bash
# 编译时启用覆盖率
g++ --coverage -std=c++20 src/*.cpp

# 运行程序生成覆盖率数据
./a.out

# 生成覆盖率报告
gcov src/*.cpp
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_report
```

**OpenCppCoverage**:
```bash
# Windows 覆盖率分析
OpenCppCoverage.exe --sources src -- -- your_program.exe

# 生成 HTML 报告
OpenCppCoverage.exe --export_type=html:coverage_report --sources src -- -- your_program.exe
```

## 3. 评分标准
> **说明**：定义各项评估指标的评分标准

### 3.1 评分等级定义

| 评分等级 | 分数范围 | 描述 | 行动建议 |
|---------|---------|------|---------|
| 优秀 | 90-100 | 完全符合要求，无改进空间 | 保持现状，作为标杆 |
| 良好 | 80-89 | 基本符合要求，有少量改进空间 | 小幅优化 |
| 合格 | 70-79 | 基本满足要求，有明显改进空间 | 需要优化 |
| 待改进 | 60-69 | 部分不符合要求，需要改进 | 必须改进 |
| 不合格 | 0-59 | 严重不符合要求，需要重构 | 立即重构 |

### 3.2 模块化评估评分标准

**模块架构分析** (权重: 20%):
- 模块分层架构符合性 (5层模型): 40分
- 模块类型分类正确性 (Public/Internal/Test): 30分
- 2层模型遵循度 (Logical Modules vs Physical Libs): 30分

**模块接口设计评估** (权重: 20%):
- 接口与实现分离 (.ixx vs .cpp): 50分
- Global Module Fragment 使用规范: 20分
- 接口文档完整性: 30分

**模块实现评估** (权重: 10%):
- 实现细节封装 (Internal Partition/Module): 50分
- 代码质量与可维护性: 50分

**评分计算**:
```
模块化评估总分 = (模块架构分析得分 × 0.20) +
                  (模块接口设计评估得分 × 0.20) +
                  (模块实现评估得分 × 0.10)
```

### 3.3 依赖分析评分标准

**模块依赖关系** (权重: 15%):
- 依赖关系清晰度: 40分
- 依赖深度合理性: 30分
- 依赖矩阵完整性: 30分

**依赖规则检查** (权重: 15%):
- 依赖单向性: 50分
- 分层架构符合性: 50分

**评分计算**:
```
依赖分析总分 = (模块依赖关系得分 × 0.15) +
               (依赖规则检查得分 × 0.15)
```

### 3.4 BMI 优化评估评分标准

**BMI 文件分析** (权重: 10%):
- 接口定义精简度 (只包含声明): 40分
- 增量构建友好度 (修改.cpp不触发重编): 40分
- BMI 生成时间效率: 20分

**评分计算**:
```
BMI 优化评估总分 = BMI 文件分析得分 × 0.10
```

### 3.5 测试隔离评估评分标准

**测试模块分析** (权重: 10%):
- 测试模块结构合理性: 40分
- 测试模块隔离性: 30分
- 测试辅助模块完整性: 30分

**测试覆盖评估** (权重: 5%):
- 测试类型覆盖: 40分
- 测试场景覆盖: 30分
- 测试覆盖率: 30分

**评分计算**:
```
测试隔离评估总分 = (测试模块分析得分 × 0.10) +
                   (测试覆盖评估得分 × 0.05)
```

### 3.6 性能评估评分标准

**编译性能评估** (权重: 5%):
- 编译时间效率: 40分
- BMI 生成时间效率: 30分
- 编译优化效果: 30分

**运行时性能评估** (权重: 5%):
- 关键函数性能: 40分
- 内存使用效率: 30分
- 并发性能: 30分

**评分计算**:
```
性能评估总分 = (编译性能评估得分 × 0.05) +
               (运行时性能评估得分 × 0.05)
```

### 3.7 代码质量评估评分标准

**内存管理与资源安全** (权重: 10%):
- 智能指针使用: 30分
- RAII 遵循: 40分
- 资源所有权明确: 30分

**异常安全评估** (权重: 5%):
- 异常保证: 40分
- 异常处理: 30分
- 资源清理: 30分

**线程安全评估** (权重: 5%):
- 线程安全设计: 40分
- 并发控制: 30分
- 死锁检测: 30分

**现代C++特性使用评估** (权重: 5%):
- C++20 特性使用: 40分
- C++17 特性使用: 30分
- C++14 特性使用: 30分

**代码规范与可维护性评估** (权重: 5%):
- 命名规范: 30分
- 代码格式: 20分
- 注释文档: 30分
- 代码复杂度: 20分

**评分计算**:
```
代码质量评估总分 = (内存管理与资源安全得分 × 0.10) +
                   (异常安全评估得分 × 0.05) +
                   (线程安全评估得分 × 0.05) +
                   (现代C++特性使用评估得分 × 0.05) +
                   (代码规范与可维护性评估得分 × 0.05)
```

### 3.8 总分计算

```
总分 = (模块化评估总分 × 0.40) +
       (依赖分析总分 × 0.20) +
       (BMI 优化评估总分 × 0.10) +
       (测试隔离评估总分 × 0.10) +
       (性能评估总分 × 0.10) +
       (代码质量评估总分 × 0.10)
```

**总分权重分布**:
- 模块化评估: 40%
- 依赖分析: 20%
- BMI 优化评估: 10%
- 测试隔离评估: 10%
- 性能评估: 10%
- 代码质量评估: 10%

**注意**: 各项评分总和为 100%，取消了风险评估作为独立项（将风险识别融入各评估环节）
