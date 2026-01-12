module;
#include <string>
#include <vector>

export module foundation.infrastructure.file_system;

export namespace foundation::infrastructure::file_system {
void remove_file(const std::string& path);
void remove_files(const std::vector<std::string>& paths);
void copy_file(const std::string& source, const std::string& destination);
inline void copy(const std::string& source, const std::string& destination) {
    copy_file(source, destination);
}
void copy_files(const std::vector<std::string>& sources, const std::string& destination);
bool file_exists(const std::string& path);
bool dir_exists(const std::string& path);
bool is_file(const std::string& path);
bool is_dir(const std::string& path);
void create_directories(const std::string& path);
inline void create_dir(const std::string& path) {
    create_directories(path);
}
std::string parent_path(const std::string& path);
std::string get_base_name(const std::string& path);
std::string get_file_name(const std::string& path);
std::string get_file_ext(const std::string& path);
std::string absolute_path(const std::string& path);
std::string utf8_to_sys_default_local(const std::string& str);
} // namespace foundation::infrastructure::file_system
