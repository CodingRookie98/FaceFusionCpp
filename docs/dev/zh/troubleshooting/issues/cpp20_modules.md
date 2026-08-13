# Issue: C++20 Modules 不被支持 (Compiler Support)

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-TS-ISSUE-CPP20-2026
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


## 问题描述
编译时报错提示无法识别模块语义，或提示 `ixx` 文件无法被处理。

## 根因分析
- **编译器版本过低**: C++20 模块是较新的特性。MSVC 需要 17.10+，GCC 需要 13+。
- **CMake 设置缺失**: 较早版本的 CMake 缺少对特定编译器模块扫描功能的支持。

## 解决方案
- **升级工具链**: 确保使用符合 [环境搭建指南](../../guides/setup.md) 要求的最新编译器。
- **启用实验性支持**: 在 Linux 下使用 GCC 时，确保 CMake 中开启了正确的模块处理标志。

## 相关链接
- [环境搭建指南](../../guides/setup.md)
