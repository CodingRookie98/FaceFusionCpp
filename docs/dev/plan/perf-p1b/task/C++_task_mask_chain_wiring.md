# C++ 任务: mask 链路接线 + 共享（P1-2 方案 B）

> **所属计划**: [IMPLEMENTATION_PLAN.md](../IMPLEMENTATION_PLAN.md) 任务 T1
> **状态**: 进行中

## 目标

接通 Box/Occlusion/Region mask 链路：config → FaceAnalysisProcessor 计算（512 参考尺度）→ FrameData 共享 → 两个 adapter 缩放复用。默认 Box 保底，Occlusion/Region 可配置启用。

## 背景

- MaskCompositor（Box/Occlusion/Region min 融合）与两个 masker 实现完整，但**链路未接线**（评估报告 P1-2 证据链）：
  - `ProcessorContext::occluder/region_masker`（runner_types.cpp:55-57）从未赋值；
  - FaceAnalysisProcessor 不设置 mask_options（默认 {box}）；
  - config `face_masker.types` 未接入 pipeline。
- 设计意图（用户确认）：默认 box；配置其他项后取 min 值；**共享**——512 尺度算一次 mask，adapter 缩放复用（避免每 adapter 各自推理）。

## TDD 流程

### 🔴 Red: 编写失败测试

**文件**: `tests/unit/services/pipeline/` 新增 `face_analysis_mask_test.cpp`（gmock occluder/region_masker）

用例：
1. `MaskComputedOnceAndShared`：FaceAnalysisProcessor 配置启用 occlusion+region → 单帧 process → occluder/region_masker 各调用 1 次；FrameData 的 mask 缓存含与 landmarks 等长的 masks。
2. `MaskSkippedWhenBoxOnly`：默认 {box} → occluder/region_masker **不调用**；FrameData 无 mask 缓存（零额外推理）。
3. `AdapterReusesSharedMask`：SwapperAdapter/FaceEnhancerAdapter 消费 FrameData 共享 mask（缩放至自身尺寸），不再各自 compose（masker 调用次数不增）。

**辅助设计**:
- `FrameData` 增加 `std::optional<FaceMaskCache> mask_cache`（`FaceMaskCache{std::vector<cv::Mat> masks; int reference_size;}`，定义于 domain.pipeline:types）。
- FaceAnalysisProcessor 构造参数扩展：`masker_options`（FaceMaskerConfig）+ `occluder`/`region_masker`（shared_ptr，可空）。
- Adapter 从 `frame.mask_cache` 取 mask（若有），resize 至自身尺寸后用于 paste_back。

### 🟢 Green: 实现

- **config**: `task_config.ixx` 的 `FaceMaskerConfig::types` 默认改为 `{"box"}`（默认零开销；task.yaml 示例可显式配置启用）。
- **domain.pipeline:types**: 增加 `FaceMaskCache`；`FrameData` 增加 `mask_cache` 字段。
- **FaceAnalysisProcessor**: 若 masker types 含 occlusion/region 且 occluder/region_masker 非空——对每张脸 warp（Ffhq512 参考）→ MaskCompositor::compose → 存 `frame.mask_cache`。
- **PipelineRunner::AddProcessorsToPipeline**: 按 `task_config.face_analysis.face_masker.types` 惰性创建 occluder/region_masker（model_repo ensure_model）→ 赋值 `context.occluder/region_masker` → 传 masker 配置给 FaceAnalysisProcessor。
- **SwapperAdapter/FaceEnhancerAdapter**: `process()` 优先取 `frame.mask_cache`（按脸索引，resize 至自身尺寸），无则回退现有 compose 逻辑。

### 🔵 Refactor

- mask 计算收敛为 FaceAnalysisProcessor 内部辅助（`compose_shared_masks`），保持 process() 主线可读。
- adapter 的 mask 获取提取公共辅助（`resolve_face_mask(frame, face_index, target_size)`）。

## 验收标准

- [ ] 失败测试先行编写（Red 确认）
- [ ] 编译通过（无警告）
- [ ] `face_analysis_mask_tests` 全绿；`domain_face_tests`/`domain_pipeline_tests` 无回归
- [ ] 默认任务零额外推理（box only）；启用后每脸仅 2 次分割推理且两 adapter 共享
- [ ] 集成/E2E 无回归

## 提交信息

```bash
feat(mask): wire occlusion/region mask chain with shared per-face masks
```