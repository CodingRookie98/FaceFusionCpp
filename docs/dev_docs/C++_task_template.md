# {任务名称} 实施任务单

> **标准参考**:
> *   架构与模块规范: [C++20 Modules 大型工程实践](../../C++/C++_20_模块-大型工程实践.md)
> *   质量与评估标准: [C++代码质量与评估标准指南](../C++_quality_standard.md)

## 1. 任务概览

### 1.1 目标
> @brief 一句话描述本任务的具体产出（如：实现 Order 模块的 Pricing 分区）

*   **目标**: {目标描述}
*   **所属计划**: [计划名称](../plan/C++_plan_{plan_name}.md)
*   **优先级**: {High/Medium/Low}

### 1.2 模块变更清单 (关键)
> **规范**: 明确本任务涉及的具体模块文件。

| 模块类型 | 文件名 | 变更类型 | 说明 |
| :--- | :--- | :--- | :--- |
| **Public Interface** | `domain.order.ixx` | Modify | 增加 `export import :pricing` |
| **Partition Interface** | `domain.order_pricing.ixx` | **New** | 定义 `PricingService` 接口 |
| **Implementation** | `domain.order_pricing.cpp` | **New** | 实现定价逻辑 |
| **Test Support** | `domain.order.test_support.ixx`| Modify | 暴露内部定价系数供测试 |

---

## 2. 详细设计与实现

### 2.1 接口定义 (Interface / .ixx)
> **BMI 优化**: 仅包含 `export` 声明，**严禁**包含非模板函数体。

```cpp
// domain.order_pricing.ixx
export module domain.order:pricing;

import domain.common;

namespace domain::order {
    export struct PriceInfo {
        double amount;
        std::string currency;
    };

    export class PricingService {
    public:
        // 仅声明
        PriceInfo calculate(int orderId);
    };
}
```

### 2.2 实现逻辑 (Implementation / .cpp)
> **物理隔离**: 实现细节放入 `.cpp`，修改不应触发下游重编。

```cpp
// domain.order_pricing.cpp
module domain.order; // 属于主模块

import <cmath>; // 依赖标准库

namespace domain::order {
    PriceInfo PricingService::calculate(int orderId) {
        // 具体实现...
        return { 100.0, "USD" };
    }
}
```

### 2.3 单元测试策略
> **测试隔离**: 使用 `test_support` 模块或 Public API 进行测试。

*   **测试文件**: `tests/unit/domain/order/test_pricing.cpp`
*   **测试用例**:
    1.  `Calculate_StandardOrder_ReturnsCorrectPrice`
    2.  `Calculate_InvalidId_ThrowsException`

---

## 3. 验证与验收

### 3.1 开发者自测 (Checklist)

*   [ ] **编译通过**: MSVC / Ninja 构建成功，无 Error。
*   [ ] **BMI 检查**: 确认修改 `.cpp` 后，下游模块（如 `service.trade`）**未**发生重新编译。
*   [ ] **测试通过**: 对应的单元测试 (`test_pricing.exe`) 100% 通过。
*   [ ] **静态分析**: Clang-Tidy 无高危警告。

### 3.2 依赖检查
*   [ ] **无反向依赖**: `domain` 层未引用 `app` 或 `service` 层代码。
*   [ ] **无循环依赖**: 编译过程未报 Circular Dependency 错误。

---

## 4. 问题记录 (Issue Log)

| 问题描述 | 解决方案 | 状态 |
| :--- | :--- | :--- |
| {如：MSVC 对某个 C++20 语法支持不完善} | {改为使用标准替代写法} | Solved |

---
**执行人**: {姓名}
**开始日期**: {YYYY-MM-DD}
**完成日期**: {YYYY-MM-DD}
