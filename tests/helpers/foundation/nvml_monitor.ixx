module;
#ifdef HAVE_NVML
#include <nvml.h>
#endif
#include <stdexcept>
#include <cstdint>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <string>

export module tests.helpers.foundation.nvml_monitor;

export namespace tests::helpers::foundation {

/**
 * @brief RAII NVML GPU Memory Monitor
 * 
 * Samples GPU memory usage in a background thread and tracks peak usage.
 */
class NvmlMonitor {
public:
    NvmlMonitor(unsigned int device_index = 0, 
                std::chrono::milliseconds sample_interval = std::chrono::milliseconds(100))
#ifdef HAVE_NVML
        : device_index_(device_index), 
          sample_interval_(sample_interval),
          running_(false),
          peak_used_bytes_(0),
          device_(nullptr) {
        
        nvmlReturn_t result = nvmlInit();
        if (result != NVML_SUCCESS) {
            std::cerr << "NVML Init failed: " << nvmlErrorString(result) << std::endl;
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
#else
    {}
#endif
    
    ~NvmlMonitor() {
#ifdef HAVE_NVML
        stop();
        nvmlShutdown();
#endif
    }
    
    void start() {
#ifdef HAVE_NVML
        if (running_) return;
        running_ = true;
        peak_used_bytes_ = 0;
        sample_thread_ = std::thread(&NvmlMonitor::sample_loop, this);
#endif
    }
    
    void stop() {
#ifdef HAVE_NVML
        if (!running_) return;
        running_ = false;
        if (sample_thread_.joinable()) {
            sample_thread_.join();
        }
#endif
    }
    
    uint64_t get_peak_used_bytes() const { 
#ifdef HAVE_NVML
        return peak_used_bytes_; 
#else
        return 0;
#endif
    }
    double get_peak_used_mb() const { return get_peak_used_bytes() / (1024.0 * 1024.0); }
    double get_peak_used_gb() const { return get_peak_used_bytes() / (1024.0 * 1024.0 * 1024.0); }
    
    uint64_t get_current_used_bytes() const {
#ifdef HAVE_NVML
        nvmlMemory_t memory;
        if (nvmlDeviceGetMemoryInfo(device_, &memory) == NVML_SUCCESS) {
            return memory.used;
        }
#endif
        return 0;
    }

private:
#ifdef HAVE_NVML
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
#endif
};

} // namespace tests::helpers::foundation
