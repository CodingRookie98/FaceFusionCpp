# Issue: TensorRT Myelin Crash on Exit

## Description
The process crashes during the exit phase after all test cases pass. The error often contains `Myelin free callback called with invalid MyelinAllocator`.

## Root Cause Analysis
- **Object Lifecycle Conflict**: TensorRT Myelin uses asynchronous callbacks to free resources. During the static object destruction phase after `main()` returns, the CUDA context might already be destroyed, while the Myelin callback still attempts to access the allocator, leading to a memory access violation.

## Solution
- **Force Sync and Cleanup**: Explicitly call `cudaDeviceSynchronize()` during the `TearDown` phase to ensure all async tasks are complete.
- **Forced Exit**: After manual resource release, utilize `_exit(0)` to bypass the static destruction phase that causes the conflict.

## Related Links
- [Resource Management in Architecture](../architecture/layers.md#56-graceful-shutdown)
