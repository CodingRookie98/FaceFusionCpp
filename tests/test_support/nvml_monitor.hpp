#pragma once

#ifdef HAVE_NVML
#include <nvml.h>
#include <stdexcept>
#include <cstdint>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

namespace test_support {

/**
 * @brief RAII NVML GPU Memory Monitor
 * 
 * Samples GPU memory usage in a background thread and tracks peak usage.
 */
class NvmlMonitor {
public:
    NvmlMonitor(unsigned int device_index = 0, 
                std::chrono::milliseconds sample_interval = std::chrono::milliseconds(100))
        : device_index_(device_index), 
          sample_interval_(sample_interval),
          running_(false),
          peak_used_bytes_(0),
          device_(nullptr) {
        
        nvmlReturn_t result = nvmlInit();
        if (result != NVML_SUCCESS) {
            std::cerr << "NVML Init failed: " << nvmlErrorString(result) << std::endl;
            // Don't throw here to allow fallback/graceful failure in tests if NVML is flaky
            // But strict requirement says we need it. 
            // Let's stick to the plan which throws.
            throw std::runtime_error("Failed to initialize NVML: " + 
                                     std::string(nvmlErrorString(result)));
        }
        
        result = nvmlDeviceGetHandleByIndex(device_index_, &device_);
        if (result != NVML_SUCCESS) {
            nvmlShutdown();
            throw std::runtime_error("Failed to get device handle: " + 
                                     std::string(nvmlErrorString(result)));
        }
    }
    
    ~NvmlMonitor() {
        stop();
        nvmlShutdown();
    }
    
    void start() {
        if (running_) return;
        running_ = true;
        peak_used_bytes_ = 0;
        sample_thread_ = std::thread(&NvmlMonitor::sample_loop, this);
    }
    
    void stop() {
        if (!running_) return;
        running_ = false;
        if (sample_thread_.joinable()) {
            sample_thread_.join();
        }
    }
    
    uint64_t get_peak_used_bytes() const { return peak_used_bytes_; }
    double get_peak_used_mb() const { return peak_used_bytes_ / (1024.0 * 1024.0); }
    double get_peak_used_gb() const { return peak_used_bytes_ / (1024.0 * 1024.0 * 1024.0); }
    
    uint64_t get_current_used_bytes() const {
        nvmlMemory_t memory;
        if (nvmlDeviceGetMemoryInfo(device_, &memory) == NVML_SUCCESS) {
            return memory.used;
        }
        return 0;
    }

private:
    void sample_loop() {
        while (running_) {
            nvmlMemory_t memory;
            if (nvmlDeviceGetMemoryInfo(device_, &memory) == NVML_SUCCESS) {
                uint64_t current = memory.used;
                uint64_t expected = peak_used_bytes_.load();
                while (current > expected && 
                       !peak_used_bytes_.compare_exchange_weak(expected, current)) {
                    // CAS loop
                }
            }
            std::this_thread::sleep_for(sample_interval_);
        }
    }
    
    unsigned int device_index_;
    std::chrono::milliseconds sample_interval_;
    std::atomic<bool> running_;
    std::atomic<uint64_t> peak_used_bytes_;
    nvmlDevice_t device_;
    std::thread sample_thread_;
};

} // namespace test_support

#else
// Stub for systems without NVML
#include <cstdint>
#include <chrono>

namespace test_support {

class NvmlMonitor {
public:
    NvmlMonitor(unsigned int = 0, std::chrono::milliseconds = std::chrono::milliseconds(100)) {}
    void start() {}
    void stop() {}
    uint64_t get_peak_used_bytes() const { return 0; }
    double get_peak_used_mb() const { return 0; }
    double get_peak_used_gb() const { return 0; }
    uint64_t get_current_used_bytes() const { return 0; }
};

} // namespace test_support
#endif
