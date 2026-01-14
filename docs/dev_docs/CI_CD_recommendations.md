# CI/CD 开发与运行建议

为了确保 FaceFusionCpp 在持续集成（CI）和持续交付（CD）环境中的稳定性，建议遵循以下准则。

## 1. 推理引擎后端选择

在 CI 环境（如 GitHub Actions, Jenkins, GitLab CI）中，通常缺乏稳定的 GPU 支持，或者 GPU 驱动在容器/虚拟机中运行不稳定。

### 强制 CPU 推理

**建议**：在执行自动化测试步骤前，设置以下环境变量：

```bash
# Windows (PowerShell)
$env:FACEFUSION_PROVIDER="cpu"
python build.py --action test

# Linux / macOS
export FACEFUSION_PROVIDER="cpu"
python build.py --action test
```

**原因**：
- **稳定性**：避开 TensorRT 和 CUDA 驱动在析构阶段可能产生的 SEH 异常或非法访问。
- **一致性**：确保在没有 GPU 的机器上测试逻辑正确性，而不受硬件驱动缺失的影响。

## 2. 资源路径配置

CI 环境中的文件结构可能与本地开发环境不同。

### 指定 Assets 路径

如果测试框架无法自动定位到 `assets` 目录，可以显式指定：

```bash
export FACEFUSION_ASSETS_PATH="/absolute/path/to/project/assets"
```

## 3. 构建配置

### 使用 Release 配置进行集成测试

虽然 Debug 配置有助于排查逻辑问题，但在 CI 中，建议至少运行一次 Release 配置的测试，以验证编译器优化是否引入了并发或时序问题。

```bash
python build.py --config Release --action test
```

## 4. 故障排查

如果在 CI 中遇到 `Access Violation` 或退出码 `0xc0000005`：
1. 首先确认是否已设置 `FACEFUSION_PROVIDER="cpu"`。
2. 检查日志（默认输出到 `logs/app.log`）以定位崩溃发生在哪个阶段。
3. 参考 `docs/dev_docs/C++_troubleshooting.md` 中的“ONNX Runtime + TensorRT 导致的 SEH 异常”章节。
