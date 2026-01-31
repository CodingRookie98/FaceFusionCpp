module;
#include <cstdint>
#include <optional>

/**
 * @file cuda_utils.ixx
 * @brief CUDA utility functions
 */
export module foundation.infrastructure.cuda_utils;

export namespace foundation::infrastructure::cuda {

/**
 * @brief GPU memory information
 */
struct GpuMemoryInfo {
    int64_t total_mb; ///< Total VRAM in MB
    int64_t used_mb;  ///< Currently used VRAM in MB
    int64_t free_mb;  ///< Available VRAM in MB
};

/**
 * @brief Get current GPU memory usage
 * @return Memory info if CUDA available, nullopt otherwise
 */
[[nodiscard]] std::optional<GpuMemoryInfo> get_gpu_memory_info();

/**
 * @brief Check if CUDA is available
 */
[[nodiscard]] bool is_cuda_available();

} // namespace foundation::infrastructure::cuda
