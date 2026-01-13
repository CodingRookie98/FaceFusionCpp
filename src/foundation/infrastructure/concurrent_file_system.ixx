module;
#include <string>
#include <vector>

export module foundation.infrastructure.concurrent_file_system;

export namespace foundation::infrastructure::concurrent_file_system {
void remove_files(const std::vector<std::string>& paths);
void copy_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destinations);
} // namespace foundation::infrastructure::concurrent_file_system
