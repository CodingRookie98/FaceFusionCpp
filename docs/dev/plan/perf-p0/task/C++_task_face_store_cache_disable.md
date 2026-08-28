# C++ 任务: 视频路径禁用 FaceStore 整帧哈希

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T1
> **状态**: 进行中

## 目标

为 `FaceAnalyser` 增加面缓存开关，视频处理路径关闭后不再执行整帧 FNV1a 哈希（消除每帧 2 次全帧哈希的纯浪费），图像路径保留缓存能力。

## 背景

- `FaceAnalyser::get_many_faces()`（`src/domain/face/analyser/face_analyser.cpp:53-175`）每帧执行：
  - `L66` `m_face_store->is_contains(frame)` → 整帧哈希；
  - `L135/L170` `insert_faces(frame, ...)` → 再次整帧哈希。
- `FaceStore::get_key()`（`src/domain/face/face_store.cpp:161-184`）对 `frame.total()*elemSize()` 逐字节 FNV1a 哈希——1080p ≈ 6.2MB×2/帧。
- 视频帧几乎不可能重复 → 缓存永远 miss → 纯浪费；且每帧 Face 数据写入全局单例 LRU（1000 帧）污染内存。
- 缓存仅对"同一图像跨任务复用"有意义（图像/批量场景）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/domain/face/analyser/face_analyser_unit_test.cpp`（复用现有 MockFaceDetector/Landmarker/Recognizer/Classifier 基建）

用例：
1. `CacheDisabledSkipsFaceStoreAccess`：构造 FaceAnalyser（mocks + 真实 `FaceStore`），调用 `set_face_cache_enabled(false)` 后 `get_many_faces()` → 断言 `FaceStore` 中无该帧缓存（`is_contains(frame) == false`，且 store 大小不增长）。
2. `CacheEnabledStoresFaces`：默认（开启）`get_many_faces()` 后 → `FaceStore` 含该帧缓存（`is_contains(frame) == true`）——防回归。
3. `CacheToggleAffectsBehavior`：先开启再关闭 → 行为随开关切换。

### 🟢 Green: 实现

- **`src/domain/face/analyser/face_analyser.ixx`**:
  - `Options` 增加 `bool enable_face_cache = true;`（构造默认开启，向后兼容）；
  - `FaceAnalyser` 增加 `void set_face_cache_enabled(bool)` 公共方法。
- **`src/domain/face/analyser/face_analyser.cpp`**:
  - `Impl` 增加 `std::atomic<bool> m_face_cache_enabled{true}`（构造时取 options）；
  - `get_many_faces()` 中 3 处 store 交互（`is_contains`/`insert_faces` 空结果/`insert_faces` 结果）用 `if (m_face_cache_enabled)` 包裹；
  - `set_face_cache_enabled` 转发到 Impl。

### 🔵 Refactor

- store 访问逻辑收敛为内部辅助方法（如 `query_face_cache`/`store_face_cache`），保持 `get_many_faces` 主线可读。
- 保持多 worker 并发安全（atomic 开关 + FaceStore 已有 shared_mutex）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `face_analyser_unit_tests` 全绿（含新增 3 用例）；`face_store_tests` 无回归
- [ ] 视频处理路径不再写入 FaceStore（集成观察/日志验证）

## 提交信息

```bash
perf(face): add face cache toggle to skip full-frame hashing on video path
```