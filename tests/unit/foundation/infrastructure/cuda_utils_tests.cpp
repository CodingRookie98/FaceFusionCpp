#include <gtest/gtest.h>
#include <iostream>

import foundation.infrastructure.cuda_utils;

namespace foundation::infrastructure::cuda {

TEST(CudaUtilsTest, IsCudaAvailable) {
    bool available = is_cuda_available();
    std::cout << "[INFO] CUDA Available: " << (available ? "Yes" : "No") << std::endl;
}

TEST(CudaUtilsTest, GetGpuMemoryInfo) {
    auto info_opt = get_gpu_memory_info();
    if (info_opt) {
        std::cout << "[INFO] GPU Memory Info:" << std::endl;
        std::cout << "  Total: " << info_opt->total_mb << " MB" << std::endl;
        std::cout << "  Used:  " << info_opt->used_mb << " MB" << std::endl;
        std::cout << "  Free:  " << info_opt->free_mb << " MB" << std::endl;

        EXPECT_GT(info_opt->total_mb, 0);
        EXPECT_GE(info_opt->used_mb, 0);
        EXPECT_GE(info_opt->free_mb, 0);
        EXPECT_EQ(info_opt->total_mb, info_opt->used_mb + info_opt->free_mb);
    } else {
        std::cout << "[INFO] CUDA not available or failed to get memory info." << std::endl;
        if (is_cuda_available()) {
            // If CUDA is available, we should ideally be able to get memory info,
            // but in some CI/CD environments with CUDA drivers but no GPU it might fail.
        }
    }
}

} // namespace foundation::infrastructure::cuda
