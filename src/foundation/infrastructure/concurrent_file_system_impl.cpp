

module;
#include <vector>
#include <string>
#include <functional>

module foundation.infrastructure.concurrent_file_system;

import foundation.infrastructure.file_system;
import foundation.infrastructure.thread_pool;

namespace foundation::infrastructure::concurrent_file_system {

void remove_files(const std::vector<std::string>& paths) {
    auto& pool = foundation::infrastructure::thread_pool::ThreadPool::instance();
    for (const auto& path : paths) {
        pool.enqueue([path]() {
            foundation::infrastructure::file_system::remove_file(path);
        });
    }
}

void copy_files(const std::vector<std::string>& sources, const std::vector<std::string>& destinations) {
    if (sources.size() != destinations.size()) {
        // Handle error: sizes must match
        return;
    }
    auto& pool = foundation::infrastructure::thread_pool::ThreadPool::instance();
    for (size_t i = 0; i < sources.size(); ++i) {
        std::string src = sources[i];
        std::string dst = destinations[i];
        pool.enqueue([src, dst]() {
            foundation::infrastructure::file_system::copy_file(src, dst);
        });
    }
}

} // namespace foundation::infrastructure::concurrent_file_system
