
module;
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

module foundation.infrastructure.file_system;

namespace foundation::infrastructure::file_system {

void remove_file(const std::string& path) {
    if (path.empty()) return;
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

void remove_files(const std::vector<std::string>& paths) {
    for (const auto& path : paths) {
        remove_file(path);
    }
}

void copy_file(const std::string& source, const std::string& destination) {
    if (source.empty() || destination.empty()) {
        throw std::invalid_argument("Source or destination path is empty");
    }
    std::error_code ec;
    std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing, ec);
}

void copy_files(const std::vector<std::string>& sources, const std::string& destination) {
    if (sources.empty() || destination.empty()) {
        throw std::invalid_argument("Sources or destination path is empty");
    }
    for (const auto& source : sources) {
        copy_file(source, destination);
    }
}

bool file_exists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

bool dir_exists(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

bool is_file(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(path, ec);
}

bool is_dir(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_directory(path, ec);
}

void create_directories(const std::string& path) {
    if (path.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
}

std::string parent_path(const std::string& path) {
    if (path.empty()) return "";
    return std::filesystem::path(path).parent_path().string();
}

std::string get_base_name(const std::string& path) {
    if (path.empty()) return "";
    return std::filesystem::path(path).stem().string();
}

std::string get_file_name(const std::string& path) {
    if (path.empty()) return "";
    return std::filesystem::path(path).filename().string();
}

std::string get_file_ext(const std::string& path) {
    if (path.empty()) return "";
    return std::filesystem::path(path).extension().string();
}

std::string absolute_path(const std::string& path) {
    if (path.empty()) return "";
    std::error_code ec;
    return std::filesystem::absolute(path, ec).string();
}

std::string utf8_to_sys_default_local(const std::string& str) {
#ifdef _WIN32
    if (str.empty()) return "";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);

    int size_needed_ansi = WideCharToMultiByte(CP_ACP, 0, wstrTo.c_str(), (int)wstrTo.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed_ansi, 0);
    WideCharToMultiByte(CP_ACP, 0, wstrTo.c_str(), (int)wstrTo.size(), &strTo[0], size_needed_ansi, NULL, NULL);
    return strTo;
#else
    return str;
#endif
}

} // namespace foundation::infrastructure::file_system
