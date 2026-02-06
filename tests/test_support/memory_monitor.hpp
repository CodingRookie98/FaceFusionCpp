#pragma once

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

namespace test_support {

/**
 * @brief Cross-platform process memory monitor
 */
class MemoryMonitor {
public:
    struct MemoryInfo {
        int64_t rss_bytes;      // Resident Set Size (physical memory)
        int64_t vms_bytes;      // Virtual Memory Size
    };
    
    static MemoryInfo get_current_memory() {
        MemoryInfo info{0, 0};
        
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), 
                                  reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), 
                                  sizeof(pmc))) {
            info.rss_bytes = static_cast<int64_t>(pmc.WorkingSetSize);
            info.vms_bytes = static_cast<int64_t>(pmc.PrivateUsage);
        }
#else
        // Linux: Read from /proc/self/status
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
};

/**
 * @brief RAII Memory Delta Checker
 * 
 * Records memory at construction and computes delta at destruction.
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

} // namespace test_support
