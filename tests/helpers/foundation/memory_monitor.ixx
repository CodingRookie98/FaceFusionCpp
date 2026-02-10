module;
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <optional>
#include <atomic>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <fstream>
#include <unistd.h>
#endif

export module tests.helpers.foundation.memory_monitor;

export namespace tests::helpers::foundation {

class MemoryMonitor {
public:
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
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            return pmc.PrivateUsage;
        }
#elif defined(__linux__)
        long rss = 0;
        std::ifstream stat("/proc/self/statm");
        if (stat >> rss >> rss) { // 2nd value is RSS in pages
            return rss * sysconf(_SC_PAGESIZE);
        }
#endif
        return 0;
    }

private:
    std::chrono::milliseconds interval_;
    std::atomic<bool> running_;
    std::atomic<size_t> peak_usage_{0};
    std::thread monitor_thread_;
};

} // namespace tests::helpers::foundation
