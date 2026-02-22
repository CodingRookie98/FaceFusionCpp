# Issue: TensorRT Myelin 进程退出崩溃 (Process Exit Crash)

## 问题描述
测试用例全部通过后，在进程退出阶段发生 SIGSEGV 或 SIGABRT 崩溃。错误信息包含 `Myelin free callback called with invalid MyelinAllocator`。

## 根因分析
- **对象生命周期冲突**: TensorRT Myelin 引擎使用异步回调释放资源。在 `main()` 返回后的静态对象析构阶段，CUDA 上下文已被销毁，但 Myelin 的异步回调仍尝试访问分配器，导致内存访问违例。

## 解决方案
- **强制同步并清理**: 在 `TearDown` 阶段明确执行 `cudaDeviceSynchronize()` 确保所有异步任务完成。
- **强制退出**: 在完成资源手动释放后，使用 `_exit(0)` 跳过可能引起冲突的静态对象析构阶段。

## 相关链接
- [架构设计中的资源管理](../architecture/design.md#56-优雅停机)
