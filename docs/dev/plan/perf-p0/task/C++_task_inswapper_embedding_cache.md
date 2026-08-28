# C++ 任务: InSwapper embedding 变换任务级缓存

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T2
> **状态**: 进行中

## 目标

缓存 InSwapper 的 embedding→初始变换结果（O(n²) 矩阵向量乘），同一 `source_embedding` 在任务内恒定 → 每帧/每脸省 262,144 次乘加。

## 背景

- `InSwapper::prepare_input()`（`src/domain/face/swapper/impl/inswapper.cpp:111-118`）对 512 维 embedding 执行双重循环矩阵向量乘（`m_initializer_array` 为 512×512），**每帧每张脸重算**；
- `source_embedding` 与 `m_initializer_array` 在任务内恒定 → 变换结果恒定 → 应缓存；
- 视频任务中同一 embedding 被调用数千次（每帧每脸），缓存收益显著。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/domain/face/swapper/inswapper_test.cpp`（复用现有 dummy ONNX + `preload_session` mock session 基建）

用例：
1. `EmbeddingTransformComputedOncePerEmbedding`：同 `source_embedding` 调用 `swap_face()` 两次 → 断言 `embedding_transform_count() == 1`（第二次命中缓存，不再重算）。
2. `EmbeddingTransformInvalidatedOnChange`：换一个 `source_embedding` 再调用 → 计数 +1（新 embedding 重算）。
3. `SwapResultCorrectAcrossCalls`：两次同 embedding 调用的输出一致（结果正确性防回归）。

> 说明: `embedding_transform_count()` 为新增测试观测接口（docstring 注明"测试/诊断用"）。

### 🟢 Green: 实现

- **`src/domain/face/swapper/impl/inswapper.ixx`**:
  - 新增成员：`mutable std::mutex m_transform_mutex; std::vector<float> m_cached_embedding; std::vector<float> m_cached_transform; size_t m_transform_count = 0;`
  - 新增公共方法 `size_t embedding_transform_count() const;`
- **`src/domain/face/swapper/impl/inswapper.cpp`**:
  - `swap_face()` 开头：比较 `source_embedding` 与 `m_cached_embedding`（`std::equal`）→ 相同则复用 `m_cached_transform`（锁内读取）；
  - 不同则计算变换（原逻辑提取为 `compute_embedding_transform`），更新缓存并 `++m_transform_count`；
  - `prepare_input()` 改为接收**已变换**的 embedding 数据（变换移到 swap_face 缓存路径）。

### 🔵 Refactor

- 变换计算逻辑收敛为独立方法，`prepare_input` 专注图像侧预处理；
- 多 worker 并发调用安全（mutex 保护缓存读写）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `inswapper_tests` 全绿（含新增 3 用例）；`face_swapper_factory_tests` 无回归
- [ ] 同任务多帧多脸不再重复计算 embedding 变换

## 提交信息

```bash
perf(swapper): cache embedding transform across frames in InSwapper
```