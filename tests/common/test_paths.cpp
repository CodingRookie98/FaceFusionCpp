#include "test_paths.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace tests::common {

std::filesystem::path TestPaths::GetExecutableDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return std::filesystem::current_path();
#endif
}

std::filesystem::path TestPaths::GetTestOutputDir(const std::string& category) {
    auto exe_dir = GetExecutableDir();
    auto output_dir = exe_dir / "output" / "test" / category;

    if (!std::filesystem::exists(output_dir)) {
        try {
            std::filesystem::create_directories(output_dir);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Failed to create directory: " << output_dir << " Reason: " << e.what()
                      << std::endl;
        }
    }
    return output_dir;
}

} // namespace tests::common
