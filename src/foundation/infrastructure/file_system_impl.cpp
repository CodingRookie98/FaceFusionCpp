
module;
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

module foundation.infrastructure.file_system;

namespace foundation::infrastructure::file_system {

void remove_file(const std::string& path) {
    if (path.empty()) return;
    std::filesystem::remove(path);
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
    std::filesystem::copy_file(source, destination, std::filesystem::copy_options::overwrite_existing);
}

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

void create_directories(const std::string& path) {
    std::filesystem::create_directories(path);
}

} // namespace foundation::infrastructure::file_system
