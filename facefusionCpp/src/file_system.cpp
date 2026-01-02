/**
 ******************************************************************************
 * @file           : file_system.cpp
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-15
 ******************************************************************************
 */

module;
#include <iostream>
#include <random>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <future>
#ifdef _WIN32
#include <windows.h>
#endif

module file_system;
import thread_pool;
import utils;

namespace ffc::file_system {

bool file_system::file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool file_system::dir_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool file_system::is_dir(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool file_system::is_file(const std::string& path) {
    return std::filesystem::is_regular_file(path);
}

std::string file_system::get_file_name_from_url(const std::string& url) {
    std::size_t lastSlashPos = url.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        return url;
    }

    std::string fileName = url.substr(lastSlashPos + 1);
    return fileName;
}

uintmax_t file_system::get_file_size(const std::string& path) {
    if (is_file(path) && file_exists(path)) {
        return std::filesystem::file_size(path);
    }
    return 0;
}

std::unordered_set<std::string> file_system::list_files(const std::string& path) {
    std::unordered_set<std::string> filePaths;

    if (!is_dir(path)) {
        throw std::invalid_argument("Path is not a directory");
    }
    if (!dir_exists(path)) {
        throw std::invalid_argument("Directory does not exist");
    }

    try {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_regular_file(entry.status())) {
                filePaths.insert(std::filesystem::absolute(entry.path()).string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return filePaths;
}

std::string file_system::absolute_path(const std::string& path) {
    return std::filesystem::absolute(path).string();
}

std::string file_system::normalize_output_path(const std::string& target_file_path, const std::string& output_dir) {
    if (!target_file_path.empty() && !output_dir.empty()) {
        const std::string targetBaseName = get_base_name(target_file_path);
        const std::string targetExtension = get_file_ext(target_file_path);

        if (is_dir(output_dir)) {
            std::string normedPath = absolute_path(output_dir + "/" + targetBaseName + targetExtension);
            while (file_exists(normedPath)) {
                std::string suffix = utils::generate_random_str(8);
                normedPath = absolute_path(output_dir + "/" + targetBaseName + "-" + suffix + targetExtension);
            }
            return normedPath;
        }
        const std::string outputDir = parent_path(output_dir);
        const std::string outputBaseName = get_base_name(output_dir);
        const std::string outputExtension = get_file_ext(output_dir);
        if (is_dir(outputDir) && !outputBaseName.empty() && !outputExtension.empty()) {
            return absolute_path(outputDir + "/" + outputBaseName + outputExtension);
        }
    }
    return {};
}

std::vector<std::string> file_system::normalize_output_paths(const std::vector<std::string>& target_paths, const std::string& output_dir) {
    std::vector<std::future<std::string>> futures;
    for (const auto& targetPath : target_paths) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue([targetPath, output_dir] {
            return normalize_output_path(targetPath, output_dir);
        }));
    }
    std::vector<std::string> normedPaths;
    for (auto& future : futures) {
        normedPaths.push_back(future.get());
    }
    return normedPaths;
}

void file_system::create_dir(const std::string& path) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (!dir_exists(path)) {
        if (std::error_code ec; !std::filesystem::create_directories(path, ec)) {
            std::cerr << __FUNCTION__ << " Failed to create directory: " + path + " Error: " + ec.message() << std::endl;
        }
    }
}

std::string file_system::get_temp_path() {
    return std::filesystem::temp_directory_path().string();
}

std::string file_system::parent_path(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string file_system::get_file_name(const std::string& file_path) {
    std::filesystem::path pathObj(file_path);
    return pathObj.filename().string();
}

std::string file_system::get_file_ext(const std::string& file_path) {
    std::filesystem::path pathObj(file_path);
    return pathObj.extension().string();
}

std::string file_system::get_base_name(const std::string& path) {
    std::filesystem::path pathObj(path);
    if (std::filesystem::is_regular_file(path)) {
        return pathObj.stem().string();
    } else if (std::filesystem::is_directory(path)) {
        return pathObj.filename().string();
    }
    return "";
}

void file_system::remove_dir(const std::string& path) {
    try {
        std::filesystem::remove_all(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << __FUNCTION__ << " Error: " << e.what() << std::endl;
    }
}

void file_system::remove_file(const std::string& path) {
    try {
        std::filesystem::remove(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void file_system::remove_files(const std::vector<std::string>& paths, const bool& use_thread_pool) {
    if (use_thread_pool) {
        std::vector<std::future<void>> futures;
        for (const auto& path : paths) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([path]() {
                remove_file(path);
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
    } else {
        for (const auto& path : paths) {
            remove_file(path);
        }
    }
}

void file_system::copy(const std::string& source, const std::string& destination) {
    if (source == destination) return;

    // parent path of destination is not exist
    if (!is_dir(std::filesystem::path(destination).parent_path().string())) {
        create_dir(std::filesystem::path(destination).parent_path().string());
    }

    std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing);
}

void file_system::copy_files(const std::vector<std::string>& sources,
                            const std::vector<std::string>& destinations,
                            const bool& use_thread_pool) {
    if (sources.size() != destinations.size()) {
        throw std::invalid_argument("Source and destination paths must have the same size");
    }

    if (use_thread_pool) {
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < sources.size(); ++i) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([sources, destinations, i]() {
                copy(sources[i], destinations[i]);
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
    } else {
        for (size_t i = 0; i < sources.size(); ++i) {
            copy(sources[i], destinations[i]);
        }
    }
}

void file_system::move_file(const std::string& source, const std::string& destination) {
    // if parent path of destination is not exist
    if (!is_dir(std::filesystem::path(destination).parent_path().string())) {
        create_dir(std::filesystem::path(destination).parent_path().string());
    }

    // if destination is existed
    if (file_exists(destination)) {
        remove_file(destination);
    }

    std::filesystem::rename(source, destination);
}

void file_system::move_files(const std::vector<std::string>& sources,
                            const std::vector<std::string>& destination,
                            const bool& use_thread_pool) {
    if (sources.size() != destination.size()) {
        throw std::invalid_argument("Source and destination paths must have the same size");
    }

    if (use_thread_pool) {
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < sources.size(); ++i) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([sources, destination, i]() {
                move_file(sources[i], destination[i]);
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
    } else {
        for (size_t i = 0; i < sources.size(); ++i) {
            move_file(sources[i], destination[i]);
        }
    }
}

void file_system::set_local_to_utf8() {
    static std::once_flag onceFlag;
    std::call_once(onceFlag, []() {
        try {
            std::locale::global(std::locale("en_US.UTF-8")); // 设置全局 locale
        } catch (const std::runtime_error& e) {
            std::cerr << std::format("Failed to set locale: {}. Fall back to system default.", e.what());
            std::locale::global(std::locale("")); // 回退到系统默认
        }
    });
}

#ifdef _WIN32
std::string file_system::utf8_to_sys_default_local(const std::string& utf8_tsr) {
    // 计算转换为宽字符所需的缓冲区大小
    const int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, utf8_tsr.c_str(), -1, nullptr, 0);
    if (wideCharSize == 0) {
        // 处理错误
        return {};
    }

    // 分配缓冲区
    std::wstring wideStr(wideCharSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_tsr.c_str(), -1, &wideStr[0], wideCharSize);
    // 计算转换为 Local 所需的缓冲区大小
    const int LocalSize = WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (LocalSize == 0) {
        // 处理错误
        return {};
    }

    // 分配缓冲区
    std::string currentLocalStr(LocalSize, 0);
    WideCharToMultiByte(CP_ACP, 0, wideStr.c_str(), -1, &currentLocalStr[0], LocalSize, nullptr, nullptr);
    return currentLocalStr;
}
#endif

std::string get_current_path() {
    return std::filesystem::current_path().string();
}

} // namespace ffc::file_system