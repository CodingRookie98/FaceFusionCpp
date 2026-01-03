/**
 * @file file_system.ixx
 * @brief File system operations module
 * @author CodingRookie
 * @date 2026-01-04
 * @note This module provides file system utility functions
 */

module;
#include <filesystem>
#include <unordered_set>
#include <opencv2/opencv.hpp>

export module file_system;

namespace ffc::infra {
export namespace file_system { // namespace ffc::infra::file_system

/**
 * @brief Check if a file exists
 * @param path Path to the file
 * @return bool True if file exists, false otherwise
 */
bool file_exists(const std::string& path);

/**
 * @brief Check if a directory exists
 * @param path Path to the directory
 * @return bool True if directory exists, false otherwise
 */
bool dir_exists(const std::string& path);

/**
 * @brief Check if path is a directory
 * @param path Path to check
 * @return bool True if path is a directory, false otherwise
 */
bool is_dir(const std::string& path);

/**
 * @brief Check if path is a file
 * @param path Path to check
 * @return bool True if path is a file, false otherwise
 */
bool is_file(const std::string& path);

/**
 * @brief Extract file name from URL
 * @param url URL string
 * @return std::string File name extracted from URL
 */
std::string get_file_name_from_url(const std::string& url);

/**
 * @brief Get file size in bytes
 * @param path Path to the file
 * @return uintmax_t File size in bytes
 */
uintmax_t get_file_size(const std::string& path);

/**
 * @brief List all files in a directory
 * @param path Path to the directory
 * @return std::unordered_set<std::string> Set of file paths in the directory
 */
std::unordered_set<std::string> list_files(const std::string& path);

/**
 * @brief Get absolute path from relative path
 * @param path Relative or absolute path
 * @return std::string Absolute path
 */
std::string absolute_path(const std::string& path);

/**
 * @brief Normalize output path by combining target file path with output directory
 * @param target_file_path Target file path
 * @param output_dir Output directory path
 * @return std::string Normalized output path
 */
std::string normalize_output_path(const std::string& target_file_path, const std::string& output_dir);

/**
 * @brief Normalize multiple output paths
 * @param target_paths Vector of target file paths
 * @param output_dir Output directory path
 * @return std::vector<std::string> Vector of normalized output paths
 */
std::vector<std::string>
normalize_output_paths(const std::vector<std::string>& target_paths, const std::string& output_dir);

/**
 * @brief Create a directory
 * @param path Path to the directory to create
 * @note Creates parent directories if they don't exist
 */
void create_dir(const std::string& path);

/**
 * @brief Remove a directory and all its contents
 * @param path Path to the directory to remove
 */
void remove_dir(const std::string& path);

/**
 * @brief Remove a file
 * @param path Path to the file to remove
 */
void remove_file(const std::string& path);

/**
 * @brief Remove multiple files
 * @param paths Vector of file paths to remove
 * @param use_thread_pool Whether to use thread pool for parallel deletion (default: true)
 */
void remove_files(const std::vector<std::string>& paths, const bool& use_thread_pool = true);

/**
 * @brief Copy a file or directory
 * @param source Source path
 * @param destination Destination path
 */
void copy(const std::string& source, const std::string& destination);

/**
 * @brief Copy multiple files
 * @param sources Vector of source file paths
 * @param destinations Vector of destination file paths
 * @param use_thread_pool Whether to use thread pool for parallel copying (default: true)
 */
void copy_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destinations,
                const bool& use_thread_pool = true);

/**
 * @brief Move a file
 * @param source Source file path
 * @param destination Destination file path
 */
void move_file(const std::string& source, const std::string& destination);

/**
 * @brief Move multiple files
 * @param sources Vector of source file paths
 * @param destination Vector of destination file paths
 * @param use_thread_pool Whether to use thread pool for parallel moving (default: true)
 */
void move_files(const std::vector<std::string>& sources,
                const std::vector<std::string>& destination,
                const bool& use_thread_pool = true);

/**
 * @brief Get system temporary directory path
 * @return std::string Path to temporary directory
 */
std::string get_temp_path();

/**
 * @brief Get parent directory path
 * @param path File or directory path
 * @return std::string Parent directory path
 */
std::string parent_path(const std::string& path);

/**
 * @brief Get file name with extension
 * @param file_path Full file path
 * @return std::string File name with extension
 */
std::string get_file_name(const std::string& file_path);

/**
 * @brief Get file extension
 * @param file_path Full file path
 * @return std::string File extension (including dot)
 */
std::string get_file_ext(const std::string& file_path);

/**
 * @brief Get base name (file name without extension or directory name)
 * @param path File or directory path
 * @return std::string Base name without extension
 * @note For files: returns file name without extension
 * @note For directories: returns directory name
 */
std::string get_base_name(const std::string& path);

/**
 * @brief Set locale to UTF-8
 * @note This function can be called multiple times
 * @note It is recommended to call it only once at program entry
 */
void set_local_to_utf8();

#ifdef _WIN32
/**
 * @brief Convert UTF-8 string to system default local encoding (Windows only)
 * @param utf8_tsr UTF-8 encoded string
 * @return std::string String in system default local encoding
 */
std::string utf8_to_sys_default_local(const std::string& utf8_tsr);
#endif

/**
 * @brief Get current program directory path
 * @return std::string Current program directory path
 */
std::string get_current_path();
}

} // namespace ffc::infra::file_system
