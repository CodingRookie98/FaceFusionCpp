<!-- AI_CONTEXT
你是一名高级 C++ 工程师。
你的目标是实施父计划中定义的模块，且不产生副作用。
约束：必须严格遵循父计划中定义的"物理布局"和"接口设计"。
不要在此创造新的架构模式。专注于高效、整洁的 C++20 实现。
⚠️ TDD 强制：所有代码必须通过 TDD 流程产生 (🔴 Red → 🟢 Green → 🔵 Refactor)。
-->

# {任务名称} 实施任务单

> **标准参考 & 跨文档链接**:
> *   TDD 与开发规范: [AGENTS.md](../../../AGENTS.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../process/C++_quality_standard.md)
> *   所属计划: [链接至对应计划](../plan/C++_plan_{plan_name}.md) (见第 1.1 节)
> *   相关评估: [链接至对应评估](../evaluation/C++_evaluation_{title}.md) (任务完成后创建)

## 0. 任务前验证 (AI Agent 自检)
> **指令**: 在验证以下内容之前，不要编写任何代码或详细设计。

*   [ ] **父计划**: 我已阅读 `docs/dev/plan/...` 并理解架构。
*   [ ] **Target 检查**: 我已检查 `src/CMakeLists.txt` (或相关) 并确认了 Target 名称。
*   [ ] **冲突检查**: 我已验证即将创建的文件名不存在。
*   [ ] **TDD 承诺**: 我确认将严格遵循 TDD 流程，先写失败测试，再写实现代码。

## 1. 任务概览

### 1.1 目标
> @brief 一句话描述本任务的具体产出（如：实现 Order 模块的 Pricing 分区）

*   **目标**: {目标描述}
*   **所属计划**: [计划名称](../plan/C++_plan_{plan_name}.md)
*   **优先级**: {High/Medium/Low}

### 1.2 模块变更清单 (关键)
> **规范**: 明确本任务涉及的具体模块文件。
> **约束**: 确保 `New` 文件不会覆盖现有工作，除非标记为 `Modify`。

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Public Interface** | `domain.order.ixx` | Modify | 增加 `export import :pricing` |
| **Partition Interface** | `domain.order_pricing.ixx` | **New** | 定义 `PricingService` 接口 |
| **Implementation** | `domain.order_pricing.cpp` | **New** | 实现定价逻辑 |
| **Test Support** | `domain.order.test_support.ixx`| Modify | 暴露内部定价系数供测试 |

---

## 2. TDD 实现流程 (核心)

> ⚠️ **强制要求**: 本节是任务实施的核心。所有功能代码必须通过 TDD 循环产生。

### 2.1 🔴 Red: 编写失败测试
> **目标**: 先定义期望行为，测试必须能独立运行且初始状态为失败。

**测试文件**: `tests/unit/{module_path}/test_{feature}.cpp`

```cpp
// test_{feature}.cpp
#include <gtest/gtest.h>
import {module_name};

TEST({TestSuite}, {TestName}_ExpectedBehavior) {
    // Arrange: 准备测试数据
    // {待填写}

    // Act: 执行被测行为
    // {待填写}

    // Assert: 验证期望结果
    // {待填写}
    FAIL() << "TODO: 实现此测试";
}
```

**测试用例清单** (必须在写实现前完成):
| # | 测试名称 | 测试场景 | 期望结果 | 状态 |
|:--|:---------|:---------|:---------|:-----|
| 1 | `{TestName}_HappyPath` | 正常输入 | 返回预期值 | 🔴 待实现 |
| 2 | `{TestName}_EdgeCase` | 边界条件 | 正确处理 | 🔴 待实现 |
| 3 | `{TestName}_ErrorCase` | 异常输入 | 抛出异常/返回错误 | 🔴 待实现 |

**验证测试失败**:
```bash
python build.py --action test
# 确认测试编译通过但执行失败 (红色)
```

### 2.2 🟢 Green: 最小实现
> **目标**: 编写**最少量**代码使测试通过。不做过度设计，只满足当前测试需求。

#### 2.2.1 接口定义 (Interface / .ixx)
> **BMI 优化**: 仅包含 `export` 声明，**严禁**包含非模板函数体。

```cpp
// {module}.ixx
export module {module_name};

namespace {namespace} {
    export struct {StructName} {
        // 仅声明数据成员
    };

    export class {ClassName} {
    public:
        // 仅声明，无函数体
        {ReturnType} {method_name}({params});
    };
}
```

#### 2.2.2 实现逻辑 (Implementation / .cpp)
> **物理隔离**: 实现细节放入 `.cpp`，修改不应触发下游重编。

```cpp
// {module}.cpp
module {module_name};

namespace {namespace} {
    {ReturnType} {ClassName}::{method_name}({params}) {
        // 最小实现 - 仅满足测试需求
        // {待填写}
    }
}
```

**验证测试通过**:
```bash
python build.py --action test
# 确认测试全部通过 (绿色)
```

### 2.3 🔵 Refactor: 优化重构
> **目标**: 在测试保护下优化代码结构。确保重构后所有测试仍然通过。

**重构检查清单**:
*   [ ] 消除重复代码 (DRY)
*   [ ] 提取公共逻辑到辅助函数
*   [ ] 改善命名以提高可读性
*   [ ] 确保符合项目代码规范
*   [ ] **验证**: 重构后所有测试仍通过

```bash
python build.py --action test
# 确认重构后测试仍全部通过
```

### 2.4 TDD 循环迭代
> 对每个功能点重复 2.1 → 2.2 → 2.3 循环，直到所有测试用例完成。

| 迭代 # | 功能点 | 🔴 测试编写 | 🟢 实现完成 | 🔵 重构完成 |
|:-------|:-------|:-----------|:-----------|:-----------|
| 1 | {功能1} | [ ] | [ ] | [ ] |
| 2 | {功能2} | [ ] | [ ] | [ ] |
| 3 | {功能3} | [ ] | [ ] | [ ] |

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **🧪 TDD 合规**: 所有功能代码均由失败测试驱动产生
*   [ ] **编译通过**: 构建成功，无 Error。
*   [ ] **BMI 检查**: 确认修改 `.cpp` 后，下游模块（如 `service.trade`）**未**发生重新编译。
*   [ ] **测试通过**: 对应的单元测试 (`test_{feature}.exe`) 100% 通过。
*   [ ] **测试质量**: 测试是行为测试（测"做什么"）而非实现测试（测"怎么做"）
*   [ ] **静态分析**: Clang-Tidy 无高危警告。

### 3.1.1 构建与增量编译验证

> **目的**: 验证模块分离设计的有效性，确保 BMI 优化带来实际编译时间收益。

*   [ ] **基线编译**：全新构建一次，记录完整编译时间 (秒)
    ```bash
    # 清空 build 目录的 CMake 缓存
    rm build/msvc-x64-debug/CMakeCache.txt build/msvc-x64-debug/CMakeFiles -r

    # 重新配置并构建
    python build.py --action configure
    python build.py --action build
    # 记录输出日志中的 Total time: XXXs
    ```
*   [ ] **修改 .cpp 后的增量编译**：仅修改某个 `{module}.cpp` 中的函数实现
    ```bash
    # 编辑 {module}.cpp
    # 再次构建
    python build.py --action build
    # 验证以下内容：
    #   - {module}.cpp 被重编译 ✓
    #   - 下游依赖模块（import {module} 的模块）是否重编？
    #     * 若未重编 → BMI 优化成功！✅
    #     * 若重编 → 检查 {module}.ixx 是否包含实现细节
    ```
*   [ ] **修改 .ixx 后的增量编译**：修改 `{module}.ixx` 中的接口声明
    ```bash
    # 编辑 {module}.ixx
    # 再次构建
    python build.py --action build
    # 预期：下游所有依赖该模块的代码都应重编（这是正常行为）
    ```
*   [ ] **增量编译收益对比**：
    - 基线完整编译耗时: `___ 秒`
    - 仅改 .cpp 增量编译: `___ 秒` (预期 < 基线的 20%)
    - 改 .ixx 增量编译: `___ 秒` (预期接近完整编译，因为下游多)

### 3.1.2 平台特定检查

#### Windows (MSVC)

*   [ ] **MSVC C++20 模块编译无警告**
    ```bash
    # 查看 ninja 的详细编译命令，确认编译器参数
    ninja -C build/msvc-x64-debug -v | grep {module_name}

    # 验证：
    #   1. 是否包含 /std:c++20 或 /std:c++latest
    #   2. 是否出现 "warning C4150" (module 导入顺序问题) 等模块相关警告
    ```
*   [ ] **CMakeLists.txt 模块依赖顺序正确**
    ```cmake
    # 检查 src/CMakeLists.txt 或对应模块的 CMakeLists.txt
    # 验证 FILE_SET cxx_modules 中：
    #   1. 依赖方（被 import 的模块）声明在前
    #   2. 使用方（import 其他模块的模块）声明在后
    #   示例：foundation_core.ixx 应在 domain_face.ixx 之前
    ```
*   [ ] **模块接口文件 (.ixx) 编译产物检查**
    ```bash
    # 查看 build 目录中的 .ifc 文件（BMI 二进制接口）
    ls -la build/msvc-x64-debug/CMakeFiles/{target_name}.dir/*.ifc

    # 验证：修改时间是否为最新（应晚于源文件修改时间）
    ```

#### Linux (GCC/Clang)

*   [ ] **C++20 模块编译无警告**
    ```bash
    # 查看 ninja 的详细编译命令
    ninja -C build/linux-x64-debug -v | grep {module_name}

    # 验证：
    #   1. 是否包含 -std=c++20
    #   2. 是否有模块相关警告
    ```
*   [ ] **CMakeLists.txt 模块依赖顺序正确**（同 Windows 检查项）
*   [ ] **模块接口文件编译产物检查**
    ```bash
    # 查看 build 目录中的 .pcm 文件（GCC/Clang BMI）
    find build/linux-x64-debug -name "*.pcm" -o -name "*.gcm" | head -20
    ```

### 3.2 依赖检查
*   [ ] **无反向依赖**: `domain` 层未引用 `app` 或 `service` 层代码。
*   [ ] **无循环依赖**: 编译过程未报 Circular Dependency 错误。

---

## 4. 问题记录与错误追踪 (Issue Log & Error Protocol)

> **3-Strike Error Protocol**: 追踪所有尝试，绝不重复相同的失败操作。
> **Agent 注意**: 如果遇到错误，必须记录在下方表格中。尝试 3 次失败后，停止并询问用户指导。

### 4.1 三次尝试规则

```
尝试 1: 诊断并修复
  → 仔细阅读错误信息
  → 识别根本原因
  → 应用针对性修复

尝试 2: 替代方案
  → 同样的错误? 尝试不同的方法
  → 不同的工具? 不同的库?
  → 绝不重复完全相同的失败操作

尝试 3: 更广泛的重新思考
  → 质疑假设
  → 搜索解决方案
  → 考虑更新计划

3 次失败后: 上报用户
  → 解释尝试过的方法
  → 分享具体错误
  → 请求指导
```

### 4.2 问题追踪表

| #    | 问题描述 | 尝试次数 | 尝试方法 | 失败原因 | 解决方案   | 状态        |
| :--- | :------- | :------- | :------- | :------- | :--------- | :---------- |
| 1    | {问题}   | 1        | {方法}   | {原因}   | {解决方案} | Open/Solved |

---
**执行人**: {姓名}
**开始日期**: {YYYY-MM-DD}
**完成日期**: {YYYY-MM-DD}
