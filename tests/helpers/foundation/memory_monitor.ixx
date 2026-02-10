module;
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <optional>
#include <atomic>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

export module tests.helpers.foundation.memory_monitor;

export namespace tests::helpers::foundation {

class MemoryMonitor {
public:
    struct MemoryInfo {
        int64_t rss_bytes;      // Resident Set Size (physical memory)
        int64_t vms_bytes;      // Virtual Memory Size
    };

    explicit MemoryMonitor(std::chrono::milliseconds interval = std::chrono::milliseconds(100))
        : interval_(interval), running_(false) {}

    ~MemoryMonitor() {
        stop();
    }

    void start() {
        if (running_) return;
        running_ = true;
        peak_usage_ = 0;
        monitor_thread_ = std::thread([this] {
            while (running_) {
                size_t current = get_current_usage();
                size_t peak = peak_usage_;
                while (current > peak && !peak_usage_.compare_exchange_weak(peak, current)) {
                    // spin
                }
                std::this_thread::sleep_for(interval_);
            }
        });
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    size_t get_peak_usage_mb() const {
        return peak_usage_ / (1024 * 1024);
    }

    static size_t get_current_usage() {
        return get_current_memory().rss_bytes;
    }

    static MemoryInfo get_current_memory() {
        MemoryInfo info{0, 0};
        
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            info.rss_bytes = static_cast<int64_t>(pmc.WorkingSetSize);
            info.vms_bytes = static_cast<int64_t>(pmc.PrivateUsage);
        }
#elif defined(__linux__)
        std::ifstream status("/proc/self/status");
        std::string line;
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") == 0) {
                std::istringstream iss(line.substr(6));
                int64_t kb;
                iss >> kb;
                info.rss_bytes = kb * 1024;
            } else if (line.find("VmSize:") == 0) {
                std::istringstream iss(line.substr(7));
                int64_t kb;
                iss >> kb;
                info.vms_bytes = kb * 1024;
            }
        }
#endif
        return info;
    }

    static double bytes_to_mb(int64_t bytes) {
        return bytes / (1024.0 * 1024.0);
    }
    
    static double bytes_to_gb(int64_t bytes) {
        return bytes / (1024.0 * 1024.0 * 1024.0);
    }

private:
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_;
    std::atomic<size_t> peak_usage_{0};
    std::thread monitor_thread_;
};

/**
 * @brief RAII Memory Delta Checker
 */
class MemoryDeltaChecker {
public:
    MemoryDeltaChecker() : start_(MemoryMonitor::get_current_memory()) {}
    
    int64_t get_rss_delta_bytes() const {
        auto current = MemoryMonitor::get_current_memory();
        return current.rss_bytes - start_.rss_bytes;
    }
    
    double get_rss_delta_mb() const {
        return MemoryMonitor::bytes_to_mb(get_rss_delta_bytes());
    }
    
    void reset() {
        start_ = MemoryMonitor::get_current_memory();
    }

private:
    MemoryMonitor::MemoryInfo start_;
};

} // namespace tests::helpers::foundation
