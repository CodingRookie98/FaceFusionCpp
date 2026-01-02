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
#include <random>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <future>
#ifdef _WIN32
#include <windows.h>
#endif
#include <opencv2/opencv.hpp>
#include <openssl/sha.h>

module file_system;
import vision;
import ffmpeg_runner;
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

bool file_system::is_image(const std::string& path) {
    if (!is_file(path) || !file_exists(path)) {
        return false;
    }
    return cv::haveImageReader(path);
}

bool file_system::is_video(const std::string& path) {
    if (!is_file(path) || !file_exists(path)) {
        return false;
    }
    return FfmpegRunner::isVideo(path);
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

bool file_system::has_image(const std::unordered_set<std::string>& paths) {
    std::unordered_set<std::string> imagePaths;

    const bool isImg = std::ranges::all_of(paths.begin(), paths.end(), [](const std::string& path) {
        const std::string absPath = absolute_path(path);
        return is_image(path);
    });
    if (!isImg) {
        return false;
    }
    return true;
}

std::unordered_set<std::string> file_system::filter_image_paths(const std::unordered_set<std::string>& paths) {
    std::unordered_set<std::string> imagePaths;
    for (auto& path : paths) {
        if (auto absPath = absolute_path(path); is_image(absPath)) {
            imagePaths.insert(absPath);
        }
    }
    return imagePaths;
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

bool file_system::copy_image(const std::string& image_path, const std::string& destination, const cv::Size& size) {
    // 读取输入图片
    const cv::Mat inputImage = cv::imread(image_path, cv::IMREAD_UNCHANGED);
    if (inputImage.empty()) {
        std::cerr << "Could not open or find the image: " << image_path << std::endl;
        return false;
    }

    // 获取临时文件路径
    std::filesystem::path destinationPath = destination;
    if (!dir_exists(destinationPath.parent_path().string())) {
        create_dir(destinationPath.parent_path().string());
    }

    // 调整图片尺寸
    cv::Mat resizedImage;
    cv::Size outputSize = vision::restrictResolution(inputImage.size(), size);
    if (outputSize.width == 0 || outputSize.height == 0) {
        outputSize = inputImage.size();
    }

    if (outputSize.width != inputImage.size().width || outputSize.height != inputImage.size().height) {
        cv::resize(inputImage, resizedImage, outputSize);
    } else {
        if (destinationPath.extension() != ".webp") {
            copy(image_path, destinationPath.string());
            return true;
        }
        resizedImage = inputImage;
    }

    if (destinationPath.extension() == ".webp") {
        // 设置保存参数，默认无压缩
        std::vector<int> compressionParams;
        compressionParams.push_back(cv::IMWRITE_WEBP_QUALITY);
        compressionParams.push_back(100); // 设置WebP压缩质量
        if (!cv::imwrite(destinationPath.string(), resizedImage, compressionParams)) {
            return false;
        }
    }

    return true;
}

bool file_system::copy_images(const std::vector<std::string>& image_paths, const std::vector<std::string>& destinations, const cv::Size& size) {
    if (image_paths.size() != destinations.size()) {
        std::cerr << __FUNCTION__ << " The number of image paths and destinations must be equal." << std::endl;
        return false;
    }
    if (image_paths.empty() || destinations.empty()) {
        std::cerr << __FUNCTION__ << " No image paths or destination paths provided." << std::endl;
        return false;
    }

    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < image_paths.size(); ++i) {
        const std::string& imagePath = image_paths[i];
        const std::string& destination = destinations[i];
        futures.emplace_back(ThreadPool::Instance()->Enqueue([imagePath, destination, size]() {
            return copy_image(imagePath, destination, size);
        }));
    }
    for (auto& future : futures) {
        if (!future.get()) {
            return false;
        }
    }
    return true;
}

bool file_system::finalize_image(const std::string& image_path, const std::string& output_path, const cv::Size& size, const int& output_image_quality) {
    // 读取输入图像
    cv::Mat inputImage = cv::imread(image_path, cv::IMREAD_UNCHANGED);
    if (inputImage.empty()) {
        return false;
    }

    // 调整图像大小
    cv::Mat resizedImage;
    cv::Size outputSize;
    if (size.width == 0 || size.height == 0) {
        outputSize = inputImage.size();
    } else {
        outputSize = size;
    }

    if (outputSize.width != inputImage.size().width || outputSize.height != inputImage.size().height) {
        cv::resize(inputImage, resizedImage, outputSize);
    } else {
        if (output_image_quality == 100) {
            copy(image_path, output_path);
            return true;
        }
        resizedImage = inputImage;
    }

    // 设置保存参数，默认无压缩
    std::vector<int> compressionParams;
    const std::string extension = std::filesystem::path(output_path).extension().string();

    if (extension == ".webp") {
        compressionParams.push_back(cv::IMWRITE_WEBP_QUALITY);
        compressionParams.push_back(std::clamp(output_image_quality, 1, 100));
    } else if (extension == ".jpg" || extension == ".jpeg") {
        compressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
        compressionParams.push_back(std::clamp(output_image_quality, 0, 100));
    } else if (extension == ".png") {
        compressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compressionParams.push_back(std::clamp((output_image_quality / 10), 0, 9));
    }

    // 保存调整后的图像
    if (!cv::imwrite(output_path, resizedImage, compressionParams)) {
        return false;
    }

    return true;
}

bool file_system::finalize_images(const std::vector<std::string>& image_paths, const std::vector<std::string>& output_paths, const cv::Size& size, const int& output_image_quality) {
    if (image_paths.size() != output_paths.size()) {
        throw std::invalid_argument("Input and output paths must have the same size");
    }

    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < image_paths.size(); ++i) {
        futures.emplace_back(ThreadPool::Instance()->Enqueue([imagePath = image_paths[i], outputPath = output_paths[i], size, output_image_quality]() {
            try {
                return finalize_image(imagePath, outputPath, size, output_image_quality);
            } catch (const std::exception& e) {
                // 记录异常或处理异常
                std::cerr << "Exception caught: " << e.what() << std::endl;
                return false; // 返回错误状态
            } catch (...) {
                // 捕获所有其他异常
                std::cerr << "Unknown exception caught" << std::endl;
                return false; // 返回错误状态
            }
        }));
    }

    bool allSuccess = true;
    for (auto& future : futures) {
        const bool success = future.get();
        if (!success) {
            allSuccess = false;
        }
    }

    return allSuccess;
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

namespace hash {

std::string sha1(const std::string& file_path) {
    if (file_path.empty()) {
        return {};
    }
    if (!file_exists(file_path)) {
        return {};
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return "";
    }

    SHA_CTX sha1_ctx;
    if (!SHA1_Init(&sha1_ctx)) {
        std::cerr << "SHA1_Init failed" << std::endl;
        return "";
    }

    constexpr size_t buffer_size = 8192; // 8KB chunks
    std::vector<char> buffer(buffer_size);

    while (file.read(buffer.data(), buffer_size)) {
        if (!SHA1_Update(&sha1_ctx, buffer.data(), file.gcount())) {
            std::cerr << "SHA1_Update failed" << std::endl;
            return "";
        }
    }
    // Handle remaining bytes
    if (file.gcount() > 0) {
        if (!SHA1_Update(&sha1_ctx, buffer.data(), file.gcount())) {
            std::cerr << "SHA1_Update failed" << std::endl;
            return "";
        }
    }

    unsigned char sha1[SHA_DIGEST_LENGTH];
    if (!SHA1_Final(sha1, &sha1_ctx)) {
        std::cerr << "SHA1_Final failed" << std::endl;
        return "";
    }

    // Convert to hex string
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

std::string combined_sha1(std::unordered_set<std::string>& file_paths,
                         const bool& use_thread_pool) {
    if (file_paths.empty()) {
        return {};
    }

    std::vector<std::string> sha1_vec;
    if (use_thread_pool) {
        std::vector<std::future<std::string>> futures;
        for (auto& file_path : file_paths) {
            futures.emplace_back(ThreadPool::Instance()->Enqueue([file_path]() {
                return sha1(file_path);
            }));
        }

        for (auto& future : futures) {
            sha1_vec.emplace_back(future.get());
        }
    } else {
        for (auto& file_path : file_paths) {
            sha1_vec.emplace_back(sha1(file_path));
        }
    }
    std::ranges::sort(sha1_vec);

    std::string combined_sha1;
    for (const auto& sha1 : sha1_vec) {
        combined_sha1 += sha1;
    }

    // 计算 sha1 值
    unsigned char sha1[SHA_DIGEST_LENGTH];
    ::SHA1(reinterpret_cast<const unsigned char*>(combined_sha1.c_str()), combined_sha1.length(), sha1);

    // 将 sha1 值转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

} // namespace file_system::hash
} // namespace ffc::file_system