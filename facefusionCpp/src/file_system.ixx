/**
 ******************************************************************************
 * @file           : file_system.h
 * @author         : CodingRookie
 * @brief          : None
 * @attention      : None
 * @date           : 24-7-15
 ******************************************************************************
 */

module;
#include <unordered_set>
#include <opencv2/opencv.hpp>

export module file_system;

namespace ffc {
export namespace FileSystem {
bool fileExists(const std::string& path);
bool dirExists(const std::string& path);
bool isDir(const std::string& path);
bool isFile(const std::string& path);
bool isImage(const std::string& path);
bool isVideo(const std::string& path);
std::string getFileNameFromURL(const std::string& url);
uintmax_t getFileSize(const std::string& path);
std::unordered_set<std::string> listFilesInDir(const std::string& path);
std::string absolutePath(const std::string& path);
bool hasImage(const std::unordered_set<std::string>& paths);
std::unordered_set<std::string> filterImagePaths(const std::unordered_set<std::string>& paths);
std::string normalizeOutputPath(const std::string& targetPath, const std::string& outputPath);
std::vector<std::string> normalizeOutputPaths(const std::vector<std::string>& targetPaths, const std::string& outputPath);
void createDir(const std::string& path);
void removeDir(const std::string& path);
void removeFile(const std::string& path);
void removeFiles(const std::vector<std::string>& paths, const bool& use_thread_pool = true);
void copy(const std::string& source, const std::string& destination);
void copyFiles(const std::vector<std::string>& sources,
               const std::vector<std::string>& destinations,
               const bool& use_thread_pool = true);
void moveFile(const std::string& source, const std::string& destination);
void moveFiles(const std::vector<std::string>& sources,
               const std::vector<std::string>& destination,
               const bool& use_thread_pool = true);
std::string getTempPath();
std::string parentPath(const std::string& path);

// 包括扩展名的文件名
std::string getFileName(const std::string& filePath);
std::string getExtension(const std::string& filePath);

// 如果是文件，返回文件名（不包括扩展名）
// 如果是目录，返回目录名
std::string getBaseName(const std::string& path);

bool copyImage(const std::string& imagePath, const std::string& destination, const cv::Size& size = cv::Size(0, 0));
// use multi-threading to copy images to destinations
bool copyImages(const std::vector<std::string>& imagePaths, const std::vector<std::string>& destinations, const cv::Size& size = cv::Size(0, 0));
bool finalizeImage(const std::string& imagePath, const std::string& outputPath, const cv::Size& size = cv::Size(0, 0), const int& outputImageQuality = 100);
bool finalizeImages(const std::vector<std::string>& imagePaths, const std::vector<std::string>& outputPaths, const cv::Size& size = cv::Size(0, 0), const int& outputImageQuality = 100);

// This function can be called multiple times.
// It is recommended to call it only once at the program entry.
void setLocalToUTF8();

#ifdef _WIN32
std::string utf8ToSysDefaultLocal(const std::string& utf8tsr);
#endif

namespace hash {
std::string SHA1(const std::string& file_path);
std::string CombinedSHA1(std::unordered_set<std::string>& file_paths,
    const bool& use_thread_pool = true);
} // namespace hash
}

} // namespace ffc::FileSystem
