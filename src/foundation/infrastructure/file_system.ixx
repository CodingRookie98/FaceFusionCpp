module;
#include <string>
#include <vector>

export module foundation.infrastructure.file_system;

export namespace foundation::infrastructure::file_system {
void remove_file(const std::string& path);
void remove_files(const std::vector<std::string>& paths);
void copy_file(const std::string& source, const std::string& destination);
void copy_files(const std::vector<std::string>& sources, const std::string& destination);
bool file_exists(const std::string& path);
void create_directories(const std::string& path);
} // namespace foundation::infrastructure::file_system
