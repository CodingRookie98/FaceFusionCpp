module;

#include <optional>
#ifdef CUDA_ENABLED
#include <cuda_runtime.h>
#endif

module foundation.infrastructure.cuda_utils;

namespace foundation::infrastructure::cuda {

std::optional<GpuMemoryInfo> get_gpu_memory_info() {
#ifdef CUDA_ENABLED
    size_t free_bytes, total_bytes;
    cudaError_t err = cudaMemGetInfo(&free_bytes, &total_bytes);

    if (err != cudaSuccess) { return std::nullopt; }

    int64_t total_mb = static_cast<int64_t>(total_bytes / (1024 * 1024));
    int64_t free_mb = static_cast<int64_t>(free_bytes / (1024 * 1024));
    int64_t used_mb = total_mb - free_mb;

    return GpuMemoryInfo{total_mb, used_mb, free_mb};
#else
    return std::nullopt;
#endif
}

bool is_cuda_available() {
#ifdef CUDA_ENABLED
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    return (err == cudaSuccess && device_count > 0);
#else
    return false;
#endif
}

} // namespace foundation::infrastructure::cuda
