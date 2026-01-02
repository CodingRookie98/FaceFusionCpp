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
#include <filesystem>
#include <unordered_set>
#include <opencv2/opencv.hpp>

export module file_system;

namespace ffc {
export namespace file_system {
bool file_exists(const std::string& path);
bool dir_exists(const std::string& path);
bool is_dir(const std::string& path);
bool is_file(const std::string& path);
bool is_image(const std::string& path);
bool is_video(const std::string& path);
std::string get_file_name_from_url(const std::string& url);
uintmax_t get_file_size(const std::string& path);
std::unordered_set<std::string> list_files(const std::string& path);
std::string absolute_path(const std::string& path);
bool has_image(const std::unordered_set<std::string>& paths);
std::unordered_set<std::string> filter_image_paths(const std::unordered_set<std::string>& paths);
std::string normalize_output_path(const std::string& target_file_path, const std::string& output_dir);
std::vector<std::string>
normalize_output_paths(const std::vector<std::string>& target_paths, const std::string& output_dir);
void create_dir(const std::string& path);
void remove_dir(const std::string& path);
void remove_file(const std::string& path);
void remove_files(const std::vector<std::string>& paths, const bool& use_thread_pool = true);
void copy(const std::string& source, const std::string& destination);
void copy_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destinations,
                const bool& use_thread_pool = true);
void move_file(const std::string& source, const std::string& destination);
void move_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destination,
                const bool& use_thread_pool = true);
std::string get_temp_path();
std::string parent_path(const std::string& path);

// 包括扩展名的文件名
std::string get_file_name(const std::string& file_path);
std::string get_file_ext(const std::string& file_path);

// 如果是文件，返回文件名（不包括扩展名）
// 如果是目录，返回目录名
std::string get_base_name(const std::string& path);

bool copy_image(const std::string& image_path, const std::string& destination, const cv::Size& size = cv::Size(0, 0));
// use multi-threading to copy images to destinations
bool copy_images(const std::vector<std::string>& image_paths,
                 const std::vector<std::string>& destinations,
                 const cv::Size& size = cv::Size(0, 0));
bool finalize_image(const std::string& image_path, const std::string& output_path,
                    const cv::Size& size = cv::Size(0, 0), const int& output_image_quality = 100);
bool finalize_images(const std::vector<std::string>& image_paths,
                     const std::vector<std::string>& output_paths,
                     const cv::Size& size = cv::Size(0, 0), const int& output_image_quality = 100);

// This function can be called multiple times.
// It is recommended to call it only once at the program entry.
void set_local_to_utf8();

#ifdef _WIN32
std::string utf8_to_sys_default_local(const std::string& utf8_tsr);
#endif

// 获取当前程序的目录路径
std::string get_current_path();

namespace hash {
std::string sha1(const std::string& file_path);
std::string combined_sha1(std::unordered_set<std::string>& file_paths,
                          const bool& use_thread_pool = true);
} // namespace hash
}

} // namespace ffc::file_system
