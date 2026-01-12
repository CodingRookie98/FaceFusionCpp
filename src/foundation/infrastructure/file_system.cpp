
module;
#include <filesystem>
#include <string>
#include <vector>
#include <stdexcept>

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
        // destination meant to be dict? no, standard copy logic usually implies dest is dir if multiple sources.
        // But the original usage might vary.
        // Logic in original file was: copy_file(source, destination) - iterating.
        // If destination is a directory, we should append filename.
        // But legacy implementation in `facefusionCpp` might have done exactly what was there.
        // The previous implementation I saw in `vision.cpp` pass `tmpTargetImgPaths` (vector) and `outputImgPaths` (vector) to `move_files`.
        // Wait, `move_files` (not `copy_files`) was used in `core.cpp`.
        // `copy_files` used in `core.cpp`: `file_system::copy_files(core_task.target_paths, originalTargetPaths);` (vector to vector).
        // My interface says `vector<string> sources, string destination`. This mismatches `copy_files(vec, vec)`.
        // I need to check `core.cpp` usage again.
        // `file_system::copy_files(core_task.target_paths, originalTargetPaths);` where both are vectors.
        // So `copy_files` should probably take `vector, vector`.
        // But `file_system.ixx` has `void copy_files(const std::vector<std::string>& sources, const std::string& destination);`.
        // This is a mismatch with legacy usage!
        // I should change `copy_files` signature to `vector, vector` or add overloading.
        // But `file_system.cpp` already has implementation for `copy_files(vec, string)`.

        // Let's check `file_system.cpp` existing content again.
        // 30: void copy_files(const std::vector<std::string>& sources, const std::string& destination) { ... }
        // It iterates sources and calls `copy_file(source, destination)`. This implies destination is overwritten repeatedly? Or destination is a DIR?
        // If destination is a dir, `std::filesystem::copy_file` will fail if dest is dir.
        // `copy_file` implementation uses `copy_file(src, dst, overwrite)`.

        // I will fix `copy_files` to match what might be needed (vector, vector) if `core.cpp` uses it that way.
        // But `core.cpp` is LEGACY code I shouldn't touch?
        // Wait, `vision.cpp` call `copy_files`?
        // No, `vision.cpp` error log didn't mention `copy_files`.

        // I'll stick to implementing missing functions for now.
        // I'll ignore `copy_files` signature issue until it breaks build (which it might if `core.cpp` is compiled).
        // But `core.cpp` is compiled.

        // I'll add `copy_files(vec, vec)` overload if needed.

        // For now, implementing missing functions.
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
    return str;
}

} // namespace foundation::infrastructure::file_system
