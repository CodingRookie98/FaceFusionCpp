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
#ifdef _WIN32
#include <windows.h>
#endif
#include <thread_pool/thread_pool.h>
#include <opencv2/opencv.hpp>
#include <openssl/sha.h>

module file_system;
import vision;
import ffmpeg_runner;

namespace ffc {

bool FileSystem::fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool FileSystem::dirExists(const std::string& path) {
    return std::filesystem::exists(path);
}

bool FileSystem::isDir(const std::string& path) {
    return std::filesystem::is_directory(path);
}

bool FileSystem::isFile(const std::string& path) {
    return std::filesystem::is_regular_file(path);
}

bool FileSystem::isImage(const std::string& path) {
    if (!isFile(path) || !fileExists(path)) {
        return false;
    }
    return cv::haveImageReader(path);
}

bool FileSystem::isVideo(const std::string& path) {
    if (!isFile(path) || !fileExists(path)) {
        return false;
    }
    return FfmpegRunner::isVideo(path);
}

std::string FileSystem::getFileNameFromURL(const std::string& url) {
    std::size_t lastSlashPos = url.find_last_of('/');
    if (lastSlashPos == std::string::npos) {
        return url;
    }

    std::string fileName = url.substr(lastSlashPos + 1);
    return fileName;
}

uintmax_t FileSystem::getFileSize(const std::string& path) {
    if (isFile(path) && fileExists(path)) {
        return std::filesystem::file_size(path);
    }
    return 0;
}

std::unordered_set<std::string> FileSystem::listFilesInDir(const std::string& path) {
    std::unordered_set<std::string> filePaths;

    if (!isDir(path)) {
        throw std::invalid_argument("Path is not a directory");
    }
    if (!dirExists(path)) {
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

std::string FileSystem::absolutePath(const std::string& path) {
    return std::filesystem::absolute(path).string();
}

bool FileSystem::hasImage(const std::unordered_set<std::string>& paths) {
    std::unordered_set<std::string> imagePaths;

    const bool isImg = std::ranges::all_of(paths.begin(), paths.end(), [](const std::string& path) {
        const std::string absPath = absolutePath(path);
        return isImage(path);
    });
    if (!isImg) {
        return false;
    }
    return true;
}

std::unordered_set<std::string> FileSystem::filterImagePaths(const std::unordered_set<std::string>& paths) {
    std::unordered_set<std::string> imagePaths;
    for (auto& path : paths) {
        if (auto absPath = absolutePath(path); isImage(absPath)) {
            imagePaths.insert(absPath);
        }
    }
    return imagePaths;
}

std::string FileSystem::normalizeOutputPath(const std::string& targetPath, const std::string& outputPath) {
    if (!targetPath.empty() && !outputPath.empty()) {
        const std::string targetBaseName = getBaseName(targetPath);
        const std::string targetExtension = getExtension(targetPath);

        if (isDir(outputPath)) {
            std::string normedPath = absolutePath(outputPath + "/" + targetBaseName + targetExtension);
            while (fileExists(normedPath)) {
                std::string suffix = generateRandomString(8);
                normedPath = absolutePath(outputPath + "/" + targetBaseName + "-" + suffix + targetExtension);
            }
            return normedPath;
        }
        const std::string outputDir = parentPath(outputPath);
        const std::string outputBaseName = getBaseName(outputPath);
        const std::string outputExtension = getExtension(outputPath);
        if (isDir(outputDir) && !outputBaseName.empty() && !outputExtension.empty()) {
            return absolutePath(outputDir + "/" + outputBaseName + outputExtension);
        }
    }
    return {};
}

std::vector<std::string> FileSystem::normalizeOutputPaths(const std::vector<std::string>& targetPaths, const std::string& outputPath) {
    dp::thread_pool pool(std::thread::hardware_concurrency());
    std::vector<std::future<std::string>> futures;
    for (const auto& targetPath : targetPaths) {
        futures.emplace_back(pool.enqueue([targetPath, outputPath] {
            return normalizeOutputPath(targetPath, outputPath);
        }));
    }
    std::vector<std::string> normedPaths;
    for (auto& future : futures) {
        normedPaths.push_back(future.get());
    }
    return normedPaths;
}

void FileSystem::createDir(const std::string& path) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    if (!dirExists(path)) {
        if (std::error_code ec; !std::filesystem::create_directories(path, ec)) {
            std::cerr << __FUNCTION__ << " Failed to create directory: " + path + " Error: " + ec.message() << std::endl;
        }
    }
}

std::string FileSystem::getTempPath() {
    return std::filesystem::temp_directory_path().string();
}

std::string FileSystem::parentPath(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string FileSystem::getFileName(const std::string& filePath) {
    std::filesystem::path pathObj(filePath);
    return pathObj.filename().string();
}

std::string FileSystem::getExtension(const std::string& filePath) {
    std::filesystem::path pathObj(filePath);
    return pathObj.extension().string();
}

std::string FileSystem::getBaseName(const std::string& path) {
    std::filesystem::path pathObj(path);
    if (std::filesystem::is_regular_file(path)) {
        return pathObj.stem().string();
    } else if (std::filesystem::is_directory(path)) {
        return pathObj.filename().string();
    }
    return "";
}

bool FileSystem::copyImage(const std::string& imagePath, const std::string& destination, const cv::Size& size) {
    // 读取输入图片
    const cv::Mat inputImage = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (inputImage.empty()) {
        std::cerr << "Could not open or find the image: " << imagePath << std::endl;
        return false;
    }

    // 获取临时文件路径
    std::filesystem::path destinationPath = destination;
    if (!dirExists(destinationPath.parent_path().string())) {
        createDir(destinationPath.parent_path().string());
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
            copy(imagePath, destinationPath.string());
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

bool FileSystem::copyImages(const std::vector<std::string>& imagePaths, const std::vector<std::string>& destinations, const cv::Size& size) {
    if (imagePaths.size() != destinations.size()) {
        std::cerr << __FUNCTION__ << " The number of image paths and destinations must be equal." << std::endl;
        return false;
    }
    if (imagePaths.empty() || destinations.empty()) {
        std::cerr << __FUNCTION__ << " No image paths or destination paths provided." << std::endl;
        return false;
    }

    // use multi-thread
    dp::thread_pool pool(std::thread::hardware_concurrency());

    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        const std::string& imagePath = imagePaths[i];
        const std::string& destination = destinations[i];
        futures.emplace_back(pool.enqueue([imagePath, destination, size]() {
            return copyImage(imagePath, destination, size);
        }));
    }
    for (auto& future : futures) {
        if (!future.get()) {
            return false;
        }
    }
    return true;
}

bool FileSystem::finalizeImage(const std::string& imagePath, const std::string& outputPath, const cv::Size& size, const int& outputImageQuality) {
    // 读取输入图像
    cv::Mat inputImage = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
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
        if (outputImageQuality == 100) {
            copy(imagePath, outputPath);
            return true;
        }
        resizedImage = inputImage;
    }

    // 设置保存参数，默认无压缩
    std::vector<int> compressionParams;
    const std::string extension = std::filesystem::path(outputPath).extension().string();

    if (extension == ".webp") {
        compressionParams.push_back(cv::IMWRITE_WEBP_QUALITY);
        compressionParams.push_back(std::clamp(outputImageQuality, 1, 100));
    } else if (extension == ".jpg" || extension == ".jpeg") {
        compressionParams.push_back(cv::IMWRITE_JPEG_QUALITY);
        compressionParams.push_back(std::clamp(outputImageQuality, 0, 100));
    } else if (extension == ".png") {
        compressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
        compressionParams.push_back(std::clamp((outputImageQuality / 10), 0, 9));
    }

    // 保存调整后的图像
    if (!cv::imwrite(outputPath, resizedImage, compressionParams)) {
        return false;
    }

    return true;
}

bool FileSystem::finalizeImages(const std::vector<std::string>& imagePaths, const std::vector<std::string>& outputPaths, const cv::Size& size, const int& outputImageQuality) {
    if (imagePaths.size() != outputPaths.size()) {
        throw std::invalid_argument("Input and output paths must have the same size");
    }

    dp::thread_pool pool(std::thread::hardware_concurrency());
    std::vector<std::future<bool>> futures;
    for (size_t i = 0; i < imagePaths.size(); ++i) {
        futures.emplace_back(pool.enqueue([imagePath = imagePaths[i], outputPath = outputPaths[i], size, outputImageQuality]() {
            try {
                return finalizeImage(imagePath, outputPath, size, outputImageQuality);
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

void FileSystem::removeDir(const std::string& path) {
    try {
        std::filesystem::remove_all(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << __FUNCTION__ << " Error: " << e.what() << std::endl;
    }
}

void FileSystem::removeFile(const std::string& path) {
    try {
        std::filesystem::remove(path);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void FileSystem::removeFiles(const std::vector<std::string>& paths, const unsigned short& threadCnt) {
    dp::thread_pool pool(threadCnt);
    std::vector<std::future<void>> futures;
    for (const auto& path : paths) {
        futures.emplace_back(pool.enqueue([path]() {
            removeFile(path);
        }));
    }
    for (auto& future : futures) {
        future.get();
    }
}

void FileSystem::copy(const std::string& source, const std::string& destination) {
    if (source == destination) return;

    // parent path of destination is not exist
    if (!isDir(std::filesystem::path(destination).parent_path().string())) {
        createDir(std::filesystem::path(destination).parent_path().string());
    }

    std::filesystem::copy(source, destination, std::filesystem::copy_options::overwrite_existing);
}

void FileSystem::copyFiles(const std::vector<std::string>& sources,
                           const std::vector<std::string>& destination,
                           const unsigned short& threadCnt) {
    if (sources.size() != destination.size()) {
        throw std::invalid_argument("Source and destination paths must have the same size");
    }

    dp::thread_pool pool(threadCnt);
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < sources.size(); ++i) {
        futures.emplace_back(pool.enqueue([sources, destination, i]() {
            copy(sources[i], destination[i]);
        }));
    }
    for (auto& future : futures) {
        future.get();
    }
}

void FileSystem::moveFile(const std::string& source, const std::string& destination) {
    // if parent path of destination is not exist
    if (!isDir(std::filesystem::path(destination).parent_path().string())) {
        createDir(std::filesystem::path(destination).parent_path().string());
    }

    // if destination is existed
    if (fileExists(destination)) {
        removeFile(destination);
    }

    std::filesystem::rename(source, destination);
}

void FileSystem::moveFiles(const std::vector<std::string>& sources,
                           const std::vector<std::string>& destination,
                           const unsigned short& threadCnt) {
    if (sources.size() != destination.size()) {
        throw std::invalid_argument("Source and destination paths must have the same size");
    }

    dp::thread_pool pool(threadCnt);
    std::vector<std::future<void>> futures;
    for (size_t i = 0; i < sources.size(); ++i) {
        futures.emplace_back(pool.enqueue([sources, destination, i]() {
            moveFile(sources[i], destination[i]);
        }));
    }
    for (auto& future : futures) {
        future.get();
    }
}

std::string FileSystem::generateRandomString(const size_t& length) {
    if (length == 0) {
        return "";
    }
    const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::random_device rd;
    std::default_random_engine generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, characters.size() - 1);

    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[distribution(generator)];
    }

    return randomString;
}

void FileSystem::setLocalToUTF8() {
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
std::string FileSystem::utf8ToSysDefaultLocal(const std::string& utf8tsr) {
    // 计算转换为宽字符所需的缓冲区大小
    const int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, utf8tsr.c_str(), -1, nullptr, 0);
    if (wideCharSize == 0) {
        // 处理错误
        return {};
    }

    // 分配缓冲区
    std::wstring wideStr(wideCharSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8tsr.c_str(), -1, &wideStr[0], wideCharSize);
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

namespace FileSystem::hash {

std::string SHA1(const std::string& file_path) {
    if (file_path.empty()) {
        return {};
    }
    if (!fileExists(file_path)) {
        return {};
    }

    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return "";
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    const size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> file_data;
    file_data.resize(file_size);
    file.read(reinterpret_cast<char*>(file_data.data()), file_size);

    // 计算文件的 SHA1 值
    unsigned char sha1[SHA_DIGEST_LENGTH];
    // CryptoPP::SHA1().CalculateDigest(sha1.data(), reinterpret_cast<const unsigned char*>(file_path.c_str()), file_path.length());
    ::SHA1(file_data.data(), file_data.size(), sha1);

    // 将 SHA1 值转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

std::string CombinedSHA1(std::unordered_set<std::string>& file_paths,
                         const unsigned short& thread_num) {
    if (file_paths.empty()) {
        return {};
    }

    dp::thread_pool pool(thread_num);
    std::vector<std::future<std::string>> futures;
    for (auto& file_path : file_paths) {
        futures.emplace_back(pool.enqueue([file_path]() {
            return SHA1(file_path);
        }));
    }

    std::vector<std::string> sha1_vec;
    for (auto& future : futures) {
        sha1_vec.emplace_back(future.get());
    }
    std::ranges::sort(sha1_vec);

    std::string combined_sha1;
    for (const auto& sha1 : sha1_vec) {
        combined_sha1 += sha1;
    }

    // 计算 SHA1 值
    unsigned char sha1[SHA_DIGEST_LENGTH];
    ::SHA1(reinterpret_cast<const unsigned char*>(combined_sha1.c_str()), combined_sha1.length(), sha1);

    // 将 SHA1 值转换为十六进制字符串
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (const auto& byte : sha1) {
        oss << std::setw(2) << static_cast<int>(byte);
    }

    return oss.str();
}

}
// namespace FileSystem::hash
} // namespace ffc