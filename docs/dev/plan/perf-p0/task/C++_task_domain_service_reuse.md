# C++ 任务: domain 服务跨视频复用（消除每视频 ONNX 重解析）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T3
> **状态**: 已完成（commit `a61112a`）

## 目标

消除 `PipelineRunner::AddProcessorsToPipeline()` 每视频/每批次重建 swapper/enhancer/restorer 并重新解析 ONNX 初始化器的浪费，改为任务级缓存复用。

## 背景

- `AddProcessorsToPipeline()`（`src/services/pipeline/pipeline_runner.cpp:345-511`）内 `domain::pipeline::PipelineContext domain_ctx;`（L352）为**局部变量**，每视频/每图像批次调用时全部重建；
- `swapper->load_model()`（L380）→ `InSwapper::init()`（`inswapper.cpp:29-84`）**重新解析 ONNX protobuf + 512×512 FP16→FP32 转换**——即使 SessionPool 命中 session，此解析仍每次执行；
- 多视频任务、分段模式（每段重建 pipeline）放大浪费。
- Session 本身经 `InferenceSessionRegistry` 全局共享（显存不翻倍），因此提升对象复用**不增加显存**。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: 新增 `tests/unit/services/pipeline/domain_service_cache_test.cpp`

引入可单测的 `DomainServiceCache`（按 key 缓存 `shared_ptr` 的辅助类），用例：
1. `GetOrCreateReturnsSameInstanceForKey`：同 key 两次 `get_or_create` → 返回同一 `shared_ptr`（`==` 相等）。
2. `FactoryInvokedOncePerKey`：mock factory 计数 → 同 key 多次调用只触发 1 次 factory。
3. `DifferentKeysCreateSeparateInstances`：不同 key → factory 各触发 1 次，实例不同。

> 说明: `DomainServiceCache` 为新增领域服务缓存辅助类（可置于 `services/pipeline/` 或 `domain/pipeline/`），key 约定为 `{step_type}:{model_name}`。

### 🟢 Green: 实现

- **新增 `DomainServiceCache`**（含 `get_or_create(key, factory)`，内部 `std::map<std::string, std::shared_ptr<void>>` 或类型化包装 + mutex）。
- **`src/services/pipeline/pipeline_runner.cpp`**:
  - `Impl` 增加 `DomainServiceCache m_service_cache;`（任务串行执行 → 简单成员缓存即可，加 mutex 防未来并行）；
  - `AddProcessorsToPipeline()` 中 swapper/enhancer/restorer 的创建改为经缓存（key = step_type + model_name）；
  - 保留 `PipelineContext` 组装逻辑（domain_ctx 仍是局部，但 swapper 等实例从缓存取）。

### 🔵 Refactor

- 模型路径解析（`m_model_repo->ensure_model`）保持每任务执行（模型文件存在性检查成本低）；
- 明确缓存生命周期 = PipelineRunner 生命周期（每任务新建 runner → 缓存随任务销毁，跨任务不共享，避免模型参数耦合）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `domain_service_cache_tests` 全绿（3 用例）；既有 runner/web 相关测试无回归
- [ ] 多视频任务中 swapper 的 ONNX 解析只执行一次（日志/集成观察）

## 提交信息

```bash
perf(pipeline): reuse domain services across videos to avoid per-video ONNX reparse
```