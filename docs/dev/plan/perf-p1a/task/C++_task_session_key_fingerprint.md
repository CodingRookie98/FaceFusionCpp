# C++ 任务: session key 加入模型文件指纹

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T3
> **状态**: 已完成（commit `4e4d311`）

## 目标

`InferenceSessionRegistry::generate_key` 追加模型文件 `size + last_write_time`，解决模型热更新（同路径覆盖）后命中旧 session、新权重不生效的问题。

## 背景

- `inference_session_registry.cpp:38-54`（generate_key）: key = model_path + EP + Dev + TRT 选项，**不含文件内容指纹**；
- 用户更新模型文件（同路径覆盖）后 key 不变 → `get_session` 返回已加载旧权重的 session → 需重启进程才生效；
- `std::filesystem::file_size` + `last_write_time` 的 stat 开销极低（每次 get_session 调用一次）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/foundation/ai/inference_session_registry_test.cpp`（若不存在则新建；或并入 session_pool_test 相关套件）

用例：
1. `KeyChangesWhenFileMtimeChanges`：创建临时模型文件 A → `generate_key(path, opts)` 得到 key1 → 修改文件内容 + 更新 mtime（`std::filesystem::last_write_time` 手动设置）→ `generate_key(path, opts)` 得到 key2 → `key1 != key2`。
2. `KeyStableForUnchangedFile`：未修改文件 → 两次 `generate_key` 相等。
3. `KeyIncludesSizeChange`：文件 size 变化 → key 变化。

> 说明: `generate_key` 需为可测接口（当前 public? 检查）。若 private 则经 `get_session` 行为验证（preload 两次不同 session）。

### 🟢 Green: 实现

- `src/foundation/ai/inference_session_registry.cpp`:
  - `generate_key` 末尾追加 `|File:{file_size}:{last_write_time.time_since_epoch().count()}`；
  - 文件不存在时省略指纹段（保持向后兼容）。

### 🔵 Refactor

- 指纹计算提取为辅助函数 `file_fingerprint(path)`（`std::optional<std::string>`），错误路径安全（stat 失败返回 nullopt 不追加）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] registry 相关测试全绿；session_pool/inference_session 测试无回归
- [ ] 模型热更新后 get_session 返回新 session（集成观察）

## 提交信息

```bash
perf(session): include model file fingerprint in session key for hot-reload
```