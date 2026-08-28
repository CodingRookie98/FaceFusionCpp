# C++ 任务: Web 集成 flaky 修复（WS 超时放宽）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T2
> **状态**: 进行中

## 目标

消除 Web 集成测试的随机 flaky：`web_ws_test.cpp` WS 读取超时 3s 在长跑负载下不足（消息延迟），放宽至 10s。

## 背景

- 全量集成跑 2 次各有 1 例随机失败（TaskProgressEndpointWorks / UnknownTaskGetsErrorMessage），根因均为 WS 消息 3s 超时内未达（服务器日志显示服务端已正确响应）；
- 单独复现全部通过 → **时序敏感**而非逻辑缺陷；
- 修复 = 放宽超时（3s→10s），消除长跑负载下的误判。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/integration/app/web_ws_test.cpp`

用例：
1. 现有 `WsReadText(fd, msg, 3000)` 调用点（L225/L245）改传 10000ms——flaky 场景下不再误报（无新断言，属超时参数修正，Red 体现为原 flaky 复现概率下降）。

> 说明: 本任务为测试稳定性修正，Red 阶段 = 记录原 flaky 根因；Green = 超时放宽。

### 🟢 Green: 实现

- `web_ws_test.cpp:225,245`: `WsReadText(fd, msg, 2000/3000)` → `10000`（统一命名常量 `kWsReadTimeoutMs = 10000`）。

### 🔵 Refactor

- 超时常量提取，注释说明"长跑负载下 WS 消息可能延迟，超时需宽松"。

## 验收标准

- [ ] 编译通过
- [ ] `web_ws_tests` 19 例全绿
- [ ] 全量集成连续 3 次无该组 flaky

## 提交信息

```bash
test(web): relax WebSocket read timeout to 10s to fix integration flakiness
```