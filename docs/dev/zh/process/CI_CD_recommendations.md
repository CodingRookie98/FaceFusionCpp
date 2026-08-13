# CI/CD 开发与运行建议

> **文档控制信息 (Document Control)**
> - **文档标识 (Document ID)**: FFC-DEV-ZH-PROC-CICD-2026
> - **当前版本 (Version)**: V1.0.0
> - **状态 (Status)**: 正式 (Official)
> - **权威性 (Authority)**: Normative
> - **所有者 (Owner)**: 王辉
> - **审核人 (Reviewer)**: 王辉
> - **最后更新 (Last Updated)**: 2026-08-13

## 修订历史记录 (Revision History)

| 版本号 | 修订日期 | 修订人 | 审核人 | 修订描述 |
| :--- | :--- | :--- | :--- | :--- |
| **V1.0.0** | 2026-08-13 | AI Agent | 王辉 | 依据文档治理规范初始化文档控制信息与修订历史。 |


为了确保 FaceFusionCpp 在持续集成（CI）和持续交付（CD）环境中的稳定性，建议遵循以下准则。

## 1. 资源路径配置

CI 环境中的文件结构可能与本地开发环境不同。

### 指定 Assets 路径

如果测试框架无法自动定位到 `assets` 目录，可以显式指定：

```bash
export FACEFUSION_ASSETS_PATH="/absolute/path/to/project/assets"
```

## 2. 构建配置

### 使用 Release 配置进行集成测试

虽然 Debug 配置有助于排查逻辑问题，但在 CI 中，建议至少运行一次 Release 配置的测试，以验证编译器优化是否引入了并发或时序问题。

```bash
python build.py --config Release --action test
```

## 3. 故障排查
