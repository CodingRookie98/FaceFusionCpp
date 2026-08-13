# 子任务 6: make_shared/make_unique 替换

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-PLAN-TD1-T06-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Informative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |


## 基本信息
- **所属计划**: fix-tech-debt
- **优先级**: P1
- **修改文件**: `src/foundation/infrastructure/logger.cpp`, `src/foundation/ai/inference_session_registry.cpp`, `src/domain/pipeline/pipeline_adapters.cpp`, `src/foundation/infrastructure/process.cpp`, `tests/unit/domain/frame/enhancer/frame_enhancer_impl_test.cpp`
- **状态**: 已完成
- **完成时间**: 2026-05-29
- **Commit ID**: 0cd6e7e

## 目标
消除所有裸 `new` 的使用，替换为现代智能指针构造方式。

## 具体改动
1. **shared_ptr(new T()) → make_shared** (5 处)
   - `logger.cpp:40`: `std::shared_ptr<Logger>(new Logger())` → `std::make_shared<Logger>()`
   - `inference_session_registry.cpp:35`: 同上模式
   - `pipeline_adapters.cpp:63,74,100`: `std::shared_ptr<IFrameProcessor>(new XxxAdapter(...))` → `std::make_shared<XxxAdapter>(...)`
2. **new char[] → make_unique<char[]>()** (3 处)
   - `process.cpp:242,259,558`: `std::unique_ptr<char[]>(new char[buffer_size])` → `std::make_unique<char[]>(buffer_size)`
3. **测试文件 new float[]** (1 处)
   - `frame_enhancer_impl_test.cpp:136`: `new float[out_size]` → `std::make_unique<float[]>(out_size)` 并调整使用方式

## 测试策略
- 编译验证
- 所有相关单元测试通过

## 验收标准
- [ ] 编译通过
- [ ] 现有测试通过
- [ ] 无 `shared_ptr(new` 或 `new char[` 或 `new float[` 残留
